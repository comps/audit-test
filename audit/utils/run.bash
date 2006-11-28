#!/bin/bash
# =============================================================================
# (c) Copyright Hewlett-Packard Development Company, L.P., 2005
# Written by Aron Griffis <aron@hp.com>
#
#   This program is free software;  you can redistribute it and/or modify
#   it under the terms of the GNU General Public License as published by
#   the Free Software Foundation; either version 2 of the License, or
#   (at your option) any later version.
#
#   This program is distributed in the hope that it will be useful,
#   but WITHOUT ANY WARRANTY;  without even the implied warranty of
#   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See
#   the GNU General Public License for more details.
#
#   You should have received a copy of the GNU General Public License
#   along with this program;  if not, write to the Free Software
#   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
# =============================================================================
# 
# This script is responsible for:
# - system config file backup, modification and restore (e.g. auditd
#   configuration and audit rules)
# - reading a test data file and iterating through individual
#   testcases
# - tallying tests passed, failed and skipped
# - handling test output levels, which are as follows:
#     - normal: pass/fail
#     - verbose: pass/fail, debug output for failed tests
#     - debug: pass/fall, debug output for all tests
#
# The normal usage of this script is simply "./run.sh" but additional
# usage information can be retrieved with "./run.sh --help"

export PATH=/usr/local/bin:/usr/bin:/bin:/usr/sbin:/sbin
if [[ -z $TOPDIR ]]; then
    TOPDIR=$(
	while [[ ! $PWD -ef / ]]; do
	    [[ -f rules.mk ]] && { echo $PWD; exit 0; }
	    cd ..
	done
	exit 1
    ) || { echo "Can't find TOPDIR, where is rules.mk?" >&2; exit 1; }
    export TOPDIR
fi
PATH=$TOPDIR/utils:$PATH

source functions.bash

#----------------------------------------------------------------------
# global variables
#----------------------------------------------------------------------

declare -a TESTS VARIATIONS
declare -i pass fail error total
declare logging=false
declare opt_verbose=false
declare opt_debug=false
declare opt_config=run.conf
declare opt_log=run.log
declare opt_rollup=rollup.log
declare opt_timeout=30

#----------------------------------------------------------------------
# utility functions
#----------------------------------------------------------------------

function die {
    [[ -n "$*" ]] && msg "${0##*/}: $*"
    exit 1
}

function warn {
    msg "<red>Warning: $*"
}

function colorize {
    declare color
    case $* in
        '<red>'*) color=31 ;;
        '<green>'*) color=32 ;;
        '<yellow>'*) color=33 ;;
        '<blue>'*) color=34 ;;
        '<magenta>'*) color=35 ;;
        '<cyan>'*) color=36 ;;
    esac
    if [[ -n $color ]]; then
        echo -ne "\033[${color}m"
        echo -n "${*#<*>}"
        echo -e "\033[0m"
    else
        echo "$*"
    fi
}

function monoize {
    declare x
    for x in red green yellow blue magenta cyan; do
        [[ $* == \<$x\>* ]] || continue
        echo "${*#<$x>}"
        return
    done
    echo "$*"
}

function lmsg {
    $logging && monoize "$*" >>"$opt_log"
}

function llmsg {
    $logging || return
    monoize "$@" >>"$opt_log"
    monoize "$@" >>"$opt_rollup"
}

function msg {
    if [[ $1 == -nolog ]]; then
	shift
    else
	llmsg "$@"
    fi
    colorize "$@"
}

function vmsg {
    if [[ $1 == -nolog ]]; then
	shift
    else
	lmsg "$@"
    fi
    if $opt_verbose || $opt_debug; then
	colorize "$@"
    fi
}

function dmsg {
    if [[ $1 == -nolog ]]; then
	shift
    else
	lmsg "$@"
    fi
    if $opt_debug; then
	colorize "$@"
    fi
}

function prf {
    if [[ $1 == -nolog ]]; then
	shift
	declare logging=false
    fi
    printf "$(colorize "$1")" "${@:2}"
    $logging || return
    printf "$(monoize "$1")" "${@:2}" | tee -a "$opt_rollup" >>"$opt_log"
}

#----------------------------------------------------------------------
# test list manipulation
#----------------------------------------------------------------------

# make_variation(char *cmdline, char *variation)
# This is just the default implementation; it can be overridden in run.conf
function make_variation {
    echo "$1 $2"
}

# run_test(char *cmdline)
# This is just the default implementation; it can be overridden in run.conf
function run_test {
    eval "$1"
}

# +(char *cmdline, char *variations...)
# add a test case to the list
function + {
    declare t=$1 v tv
    shift

    # use default VARIATIONS if none specified
    (( $# > 0 )) || set -- "${VARIATIONS[@]}"

    # add each variation to the list
    if (( $# > 0 )); then
        for v in "$@"; do
            tv=$(make_variation "$t" "$v")
            dmsg "Adding TESTS[${#TESTS[@]}]: $tv"
            TESTS[${#TESTS[@]}]=$tv
        done
    else
        dmsg "Adding TESTS[${#TESTS[@]}]: $t"
        TESTS[${#TESTS[@]}]=$t
    fi
}

# -(char *cmdline, char *variations...)
# remove test case(s) from the list
function - {
    declare i t=$1 v tv found=false
    shift

    if [[ $t == ALL ]]; then
        TESTS=()
        return 0
    fi

    # use default VARIATIONS if none specified
    (( $# > 0 )) || set -- "${VARIATIONS[@]}"

    # remove each of the variations specified
    if (( $# > 0 )); then
        for v in "$@"; do
            for ((i = 0; i < ${#TESTS[@]}; i++)); do
                tv=$(make_variation "$t" "$v")
                if [[ "${TESTS[i]}" == "$tv" ]]; then
                    dmsg "Removing TESTS[$i]: $tv"
                    unset TESTS[i]
                    found=true
                fi
            done
        done
    else
        for ((i = 0; i < ${#TESTS[@]}; i++)); do
            if [[ "${TESTS[i]}" == "$t" ]]; then
                dmsg "Removing TESTS[$i]: $t"
                unset TESTS[i]
                found=true
            fi
        done
    fi

    if $found; then
        # remove unset tests
        TESTS=( "${TESTS[@]}" )
    else
        warn "couldn't find \"$t\""
    fi
}

#----------------------------------------------------------------------
# startup/cleanup
#----------------------------------------------------------------------

trap 'cleanup &>/dev/null; close_log; exit' 0
trap 'cleanup; close_log; exit' 1 2 3 15

# this can be overridden in run.conf
function startup_hook {
    true
}

# this can be overridden in run.conf
function cleanup_hook {
    true
}

function startup {
    export TEST_USER=testuser
    export TEST_USER_PASSWD='2manySecre+S'

    # testuser's password encrypted with perl -pe 's/.*/crypt $&, "AG"/e'
    declare passwd_encrypted=AGVR9oEWQg4.k

    dmsg "Starting up"

    # Make sure we're running as root
    if [[ $EUID != 0 ]]; then
        die "Please run this suite as root"
    fi

    # Initialize audit configuration and make sure auditd is running
    prepend_cleanup "killall -HUP auditd" # do right after restoring config
    backup "$auditd_conf"		  # backup uses prepend_cleanup
    write_config -r "$auditd_conf" dispatcher DISP_qos
    start_auditd >/dev/null || die

    # Add the test user which is used for unprivileged tests
    userdel -r $TEST_USER &>/dev/null
    groupdel $TEST_USER &>/dev/null
    dmsg "Adding group $TEST_USER"
    groupadd "$TEST_USER" || die
    dmsg "Adding user $TEST_USER"
    useradd -g "$TEST_USER" -p "$passwd_encrypted" -m "$TEST_USER" || die

    append_cleanup '
	if [[ -n $TEST_USER ]]; then
	    # Remove the test user
	    dmsg "Removing user $TEST_USER"
	    userdel -r "$TEST_USER" &>/dev/null
	    dmsg "Removing group $TEST_USER"
	    groupdel "$TEST_USER" &>/dev/null
	fi
	cleanup_hook'

    startup_hook
}

# cleanup function should send output via dmsg
eval "function cleanup {
    dmsg \"\$( $(type cleanup | sed '1,3d;$d') )\"
}"

function open_log {
    :> "$opt_log" || die "can't init $opt_log"
    :> "$opt_rollup" || die "can't init $opt_rollup"
    logging=true

    dmsg "Log file started $(date)"
}

function close_log {
    logging=false
}

#----------------------------------------------------------------------
# main program
#----------------------------------------------------------------------

function usage {
    cat <<EOF
Usage: ${0##*/} [OPTION]...
Run a set of test cases, reporting pass/fail and tallying results.

    -f --config=FILE  Use a config file other than run.conf
       --header       Don't run anything, just output the log header
    -l --log=FILE     Output to a log other than run.log
    -r --rollup=FILE  Output to a rollup other than rollup.log
    -t --timeout=SEC  Seconds to wait for a test to timeout, default 30
    -h --help         Show this help

Output modes:
    (default)         Pass/fail/error status only
    -v --verbose      Copious output on fail or error
    -d --debug        Copious output always

EOF
}

function parse_cmdline {
    declare args conf tcase

    # Use /usr/bin/getopt which supports GNU-style long options
    args=$(getopt -o df:hl:r:v \
        --long config:,debug,help,header,log:,rollup:,nocolor,verbose \
        -n "$0" -- "$@") || die
    eval set -- "$args"

    while true; do
        case $1 in
            -d|--debug) opt_debug=true; opt_verbose=true; shift ;;
            -f|--config) opt_config=$2; shift 2 ;;
            -h|--help) usage; exit 0 ;;
	    --header) header; exit 0 ;;
            -l|--log) opt_log=$2; shift 2 ;;
            -r|--rollup) opt_rollup=$2; shift 2 ;;
            -t|--timeout) opt_timeout=$2; shift 2 ;;
            --nocolor) colorize() { monoize "$@"; }; shift ;;
            -v|--verbose) opt_verbose=true; shift ;;
            --) shift ; break ;;
            *) die "failed to process cmdline args" ;;
        esac
    done

    # Open the logs now that opt_log and opt_rollup are set
    open_log

    # Load the config
    dmsg "Loading config from $opt_config"
    conf="$(<$opt_config)
          true"
    eval -- "$conf" || die "Error reading config file: $opt_config"

    # Additional cmdline indicates test adds/removes
    if [[ -n $* ]]; then
        declare remove_all=true
        dmsg "Loading additional test cases from cmdline"
        while [[ -n $1 ]]; do
            if [[ $1 == [+-]* ]]; then
                # make sure there's a space
                tcase="${1:0:1} ${1:1}"
            else
                # bare tests on the cmdline imply to start from scratch
                $remove_all && - ALL
                tcase="+ $1"
            fi
            eval -- "$tcase" || die "Error evaluating \"$tcase\""
            shift
            remove_all=false
        done
    fi
}

function header {
    prf "%12s: %s\n" Started "$(date)"
    prf "%12s: %s\n" Kernel "$(uname -r)"
    prf "%12s: %s\n" Architecture "$(uname -m)"
    prf "%12s: %s\n" Mode "${MODE:-(native)}"
    prf "%12s: %s\n" Hostname "$(uname -n)"
}

function run_tests {
    declare t output status hee
    declare begin_output="<blue>--- begin output -----------------------------------------------------------"
    declare end_output="<blue>--- end output -------------------------------------------------------------"

    header
    msg 
    prf "%-60s %s\n" "Testcase" "Result"
    prf "%-60s %s\n" "--------" "------"

    if $opt_debug; then
	hee=/dev/stderr
    else
	hee=/dev/null
    fi

    for t in "${TESTS[@]}"; do
	if $opt_debug; then
	    prf -nolog "%-60s " "$t"
	    msg -nolog "<blue>DEBUG"
	    msg -nolog "$begin_output"
	else
	    prf "%-60s " "$t"
	fi

	output=$(
# note that putting run_test in the background results in no tty for pam tests
	    ( run_test "$t" 2>&1 | tee $hee; exit ${PIPESTATUS[0]}; ) # &
#	    pid=$!
# opt_timeout is disabled for now due to a bash bug.  If the timeout is put into
# the background and $pid exits before wait is called, the wait will fail
# because bash claims $pid is not a child of this shell.
#	    if [[ $opt_timeout > 0 ]]; then
#		( sleep $opt_timeout; kill $pid; ) &>/dev/null &
#	    fi
#	    wait $pid
	) 2>&1
	status=$?

	if $opt_debug; then
	    msg -nolog "$end_output"
	    prf "%-60s " "$t"
	fi

	if [[ $status == 0 ]]; then
	    msg "<green>PASS"
	    (( pass++ ))
	    lmsg "$begin_output"
	    lmsg "$output"
	    lmsg "$end_output"
	    lmsg
	else
	    if [[ $status == 1 ]]; then
		msg "<yellow>FAIL"
		(( fail++ )) 
	    else
		msg "<red>ERROR ($status)"
		(( error++ ))
	    fi
	    if $opt_debug; then
		lmsg "$begin_output"
		lmsg "$output"
		lmsg "$end_output"
		lmsg
	    else
		vmsg "$begin_output"
		vmsg "$output"
		vmsg "$end_output"
		vmsg
	    fi
	fi
    done

    (( total = pass + fail + error ))
    msg
    prf "%4d pass (%d%%)\n" $pass $((pass * 100 / total))
    prf "%4d fail (%d%%)\n" $fail $((fail * 100 / total))
    prf "%4d error (%d%%)\n" $error $((error * 100 / total))
    prf "%s\n" "------------------"
    prf "%4d total\n" $total

    return 0
}

parse_cmdline "$@"
startup || die "startup failed"
run_tests
exit $?
