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
    exit 1
}

function warn {
    msg "Warning: $*"
}

function msg {
    $logging && echo "$*" >>"$opt_log"
    echo "$*"
}

function vmsg {
    $logging && echo "$*" >>"$opt_log"
    $opt_verbose || $opt_debug && echo "$*"
}

function dmsg {
    $logging && echo "$*" >>"$opt_log"
    $opt_debug && echo "$*"
}

function prf {
    $logging && printf "$@" >>"$opt_log"
    printf "$@"
}

#----------------------------------------------------------------------
# test list manipulation
#----------------------------------------------------------------------

# make_variation(char *cmdline, char *variation)
# This is just the default implementation; it can be overridden in run.conf
function make_variation {
    echo "$*"
}

# run_test(char *cmdline)
# This is just the default implementation; it can be overridden in run.conf
function run_test {
    eval "$*"
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
    declare u=testuser

    dmsg "Starting up"

    # Make sure we're running as root
    if [[ $EUID != 0 ]]; then
        die "Please run this suite as root"
    fi

    # Add the test user which is used for unprivileged tests
    if ! grep -q "^$u:" /etc/group; then 
        dmsg "Adding group $u"
        groupadd "$u" || exit 1
    fi
    if ! grep -q "^$u:" /etc/passwd; then 
        dmsg "Adding user $u"
        useradd -g "$u" -m "$u" || exit 1
    fi

    # Mark the current position in the audit log
    export AUDIT_SEEK=$(wc -c /var/log/audit/audit.log 2>/dev/null)
    [[ -z $AUDIT_SEEK ]] && AUDIT_SEEK=0

    startup_hook
}

function cleanup {
    declare u=testuser

    dmsg "Cleaning up"

    # Remove the test user
    if grep -q "^$u:" /etc/passwd; then
        dmsg "Removing user $u"
        userdel "$u"
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
        --long config:,debug,help,log:,verbose \
        -n "$0" -- "$@") || die
    eval set -- "$args"

    while true; do
        case $1 in
            -d|--debug) opt_debug=true; opt_verbose=true; shift ;;
            -f|--config) opt_config=$2; shift 2 ;;
            -h|--help) usage; exit 0 ;;
            -l|--log) opt_log=$2; shift 2 ;;
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
        dmsg "Loading additional test cases from cmdline"
        while [[ -n $1 ]]; do
            # make sure there's a space
            tcase="${1:0:1} ${1:1}"
            eval -- "$tcase" || die "Error evaluating \"$tcase\""
            shift
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
            0)  echo PASS
                (( pass++ ))
                dmsg "$output"
                dmsg
                ;;

            1)  echo FAIL
                (( fail++ ))
                vmsg "$output"
                vmsg
                ;;

            *)  echo "ERROR ($?)"
                (( error++ ))
                vmsg "$output"
                vmsg
                ;;
        esac
    done


    (( total = pass + fail + error ))
    echo
    prf "%4d pass (%d%%)\n" $pass $((pass * 100 / total))
    prf "%4d fail (%d%%)\n" $fail $((fail * 100 / total))
    prf "%4d error (%d%%)\n" $error $((error * 100 / total))
    prf "%s\n" "------------------"
    prf "%4d total\n" $total

    (( error > 0 )) && return 2
    (( fail > 0 )) && return 1
    return 0
}

parse_cmdline "$@"
startup || die "startup failed"
run_tests
exit $?
