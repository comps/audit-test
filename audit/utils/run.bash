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

shopt -s extglob

#----------------------------------------------------------------------
# global variables
#----------------------------------------------------------------------

declare -a TESTS VARIATIONS
declare -i pass fail error total
declare exit_status
declare logging=false
declare opt_verbose=false
declare opt_debug=false
declare opt_config=run.conf
declare opt_log=run.log

#----------------------------------------------------------------------
# utility functions
#----------------------------------------------------------------------

function die {
    [[ -n "$*" ]] && msg "${0##*/}: $*"
    exit 2
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

function msg {
    $logging && monoize "$*" >>"$opt_log"
    colorize "$*"
}

function vmsg {
    $logging && monoize "$*" >>"$opt_log"
    $opt_verbose || $opt_debug && colorize "$*"
}

function dmsg {
    $logging && monoize "$*" >>"$opt_log"
    $opt_debug && colorize "$*"
}

function prf {
    $logging && printf "$(monoize "$1")" "${@:2}" >>"$opt_log"
    printf "$(colorize "$1")" "${@:2}"
}

#----------------------------------------------------------------------
# routines available to run_test
# NB: these should simply echo/printf, not msg/vmsg/dmsg/prf because
# run_test output is already going to the log.
#----------------------------------------------------------------------

function rotate_audit_logs {
    declare tmp num_logs

    echo "Rotating logs, one way or another"

    if [[ -f /var/log/audit/audit.log ]]; then
        pushd /var/log/audit >/dev/null
        tmp=$(mktemp $PWD/rotating.XXXXXX) || return 2
        ln -f audit.log "$tmp" || return 2
        killall -USR1 auditd &>/dev/null
        sleep 0.1

        # Forced log rotation wasn't added until 1.0.10.  If it didn't work,
        # then do it manually.
        if [[ audit.log -ef $tmp ]]; then
            service auditd stop
            num_logs=$(awk '$1=="num_logs"{print $3;exit}' /etc/auditd.conf)
            while (( --num_logs > 0 )); do
                if (( num_logs == 1 )); then
                    mv -vf audit.log audit.log.1 2>/dev/null
                else
                    mv -vf audit.log.$((num_logs-1)) audit.log.$((num_logs)) \
                        2>/dev/null
                fi
            done
        fi

        rm -f "$tmp"
        popd >/dev/null
    fi

    killall -0 auditd &>/dev/null || service auditd start
    sleep 0.1
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

trap 'cleanup; close_log; exit' 0 1 2 3 15

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
    export TEST_USER_PASSWD=gentoo    # encrypted below

    dmsg "Starting up"

    # Make sure we're running as root
    if [[ $EUID != 0 ]]; then
        die "Please run this suite as root"
    fi

    # Make sure auditd is running
    if [[ $(service auditd status) == *running* ]]; then
        service auditd start || exit 1
    fi

    # Add the test user which is used for unprivileged tests
    if ! grep -q "^$TEST_USER:" /etc/group; then 
        dmsg "Adding group $TEST_USER"
        groupadd "$TEST_USER" || exit 1
    fi
    if grep -q "^$TEST_USER:" /etc/passwd; then 
        dmsg "Removing old $TEST_USER"
        userdel "$TEST_USER" || exit 1
        rm -rf /home/$TEST_USER
    fi

    dmsg "Adding user $TEST_USER"
    useradd -g "$TEST_USER" -m "$TEST_USER" || exit 1
    sed -i "/^$TEST_USER:/"'s|:[^:]*:|:$1$N1PtB8Kg$d6gItPaB3lSpG/GiDOXEM1:|' \
        /etc/shadow

    startup_hook
}

function cleanup {
    declare u=testuser

    dmsg "Cleaning up"

    # Remove the test user
    if grep -q "^$u:" /etc/passwd; then
        dmsg "Removing user $u"
        userdel "$u"
        rm -rf "/home/$u"
    fi
    if grep -q "^$u:" /etc/group; then
        dmsg "Removing group $u"
        groupdel "$u"
    fi

    cleanup_hook
}

function open_log {
    :> "$opt_log" || die "can't init $opt_log"
    logging=true

    dmsg "Log file started $(date)"
}

function close_log {
    logging=false
}

#----------------------------------------------------------------------
# test environment
#----------------------------------------------------------------------

export PATH=/usr/local/bin:/usr/bin:/bin:/usr/sbin:/sbin
if [[ -z $TOPDIR ]]; then
    TOPDIR=$(readlink run.bash)
    TOPDIR=${TOPDIR%/utils/*}
    export TOPDIR
fi
pushd $TOPDIR >/dev/null || die "Can't access TOPDIR ($TOPDIR)"
PATH=$PWD/utils:$PATH
popd >/dev/null

#----------------------------------------------------------------------
# main program
#----------------------------------------------------------------------

function usage {
    cat <<EOF
Usage: ${0##*/} [OPTION]...
Run a set of test cases, reporting pass/fail and tallying results.

    -f --config=FILE  Use a config file other than run.conf
    -l --log=FILE     Output to a log other than run.log
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
    args=$(getopt -o df:hl:v \
        --long config:,debug,help,log:,nocolor,verbose \
        -n "$0" -- "$@") || die
    eval set -- "$args"

    while true; do
        case $1 in
            -d|--debug) opt_debug=true; opt_verbose=true; shift ;;
            -f|--config) opt_config=$2; shift 2 ;;
            -h|--help) usage; exit 0 ;;
            -l|--log) opt_log=$2; shift 2 ;;
            --nocolor) colorize() { monoize "$@"; }; shift ;;
            -v|--verbose) opt_verbose=true; shift ;;
            --) shift ; break ;;
            *) die "failed to process cmdline args" ;;
        esac
    done

    # Open the log
    if [[ -n $opt_log ]]; then
        open_log "$opt_log"
    fi

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

function run_tests {
    declare x t v output

    for t in "${TESTS[@]}"; do
        prf "%-60s " "$t"

        # run_test is defined in the configuration file
        output=$(run_test "$t" 2>&1)

        case $? in
            0)  msg "<green>PASS"
                (( pass++ ))
                dmsg "<blue>--- begin output -----------------------------------------------------------"
                dmsg "$output"
                dmsg "<blue>--- end output -------------------------------------------------------------"
                dmsg
                ;;

            1)  msg "<yellow>FAIL"
                (( fail++ ))
                vmsg "<blue>--- begin output -----------------------------------------------------------"
                vmsg "$output"
                vmsg "<blue>--- end output -------------------------------------------------------------"
                vmsg
                ;;

            *)  msg "<red>ERROR ($?)"
                (( error++ ))
                vmsg "<blue>--- begin output -----------------------------------------------------------"
                vmsg "$output"
                vmsg "<blue>--- end output -------------------------------------------------------------"
                vmsg
                ;;
        esac
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
