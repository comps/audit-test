#!/bin/bash
# =============================================================================
# (c) Copyright Hewlett-Packard Development Company, L.P., 2005
# Written by Aron Griffis <aron@hp.com>
#
#   This program is free software: you can redistribute it and/or modify
#   it under the terms of version 2 the GNU General Public License as
#   published by the Free Software Foundation.
#   
#   This program is distributed in the hope that it will be useful,
#   but WITHOUT ANY WARRANTY; without even the implied warranty of
#   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#   GNU General Public License for more details.
#   
#   You should have received a copy of the GNU General Public License
#   along with this program.  If not, see <http://www.gnu.org/licenses/>.
# =============================================================================
#
# functions.bash: routines available to run_test and to bash test cases
#
# NB: these should simply echo/printf, not msg/vmsg/dmsg/prf because
#     run_test output is already going to the log.

[[ -z ${_FUNCTIONS_BASH} ]] || return 0
_FUNCTIONS_BASH=1

shopt -s extglob

######################################################################
# global vars
######################################################################

unset auditd_conf audit_log
auditd_conf=/etc/audit/auditd.conf
audit_log=/var/log/audit/audit.log

unset zero
zero=${0##*/}

######################################################################
# utility functions
######################################################################

# write values to config files
# write_config [-sc] <config file> <key[=value]>...
# 	-r	remove line for specified key, instead of replacing
# 	-s	use spaces for key/value pairs
function write_config {
    declare x key value config=$1 dospace=false remove=false

    args=$(getopt -o rs -n "$0" -- "$@")
    eval set -- "$args"
    while true; do
	case $1 in
	    -r) remove=true; shift ;;
	    -s) dospace=true; shift ;;
	    --) shift; break ;;
	     *) echo "write_config: failed to process cmdline args"; return 2 ;;
	esac
    done

    config=$1
    shift

    # Remove or replace configuration lines; slow but works
    for x in "$@"; do
	key=${x%%=*}
	value=${x#*=}

	sed -i "/^$key[[:blank:]]*=/d" "$config"
	[[ $remove == true ]] && return 0

	if $dospace; then
            echo "$key = $value" >> "$config"
        else
            echo "$key=$value" >> "$config"
        fi
    done
}

# exit_pass, exit_fail and exit_error are used in run_test.  They could live in
# testcase.bash but we define them here so that exit_error can be called if
# source testcase.bash fails.
function exit_pass { [[ -n $* ]] && printf "\nexit_pass: %s\n" "$*" >&2; exit 0; }
function exit_fail { [[ -n $* ]] && printf "\nexit_fail: %s\n" "$*" >&2; exit 1; }
function exit_error { 
    declare status
    if [[ -n $1 && $1 != *[!0-9]* && $1 -ge 2 ]]; then
	status=$1  # use error status given
	shift
    else
	status=2   # default error status
    fi
    [[ -n $* ]] && printf "\nexit_error: %s\n" "$*" >&2
    exit $status
}

# parse_named - Parse key=value test arguments
#
# INPUT
# "$@" : test command line arguments
#
# OUTPUT
# Bash command string to declare and set variables based on the named arguments.
#
# DESCRIPTION
# This function is passed a test command line, and outputs bash commands which
# declare and set variables based on the "name=value" syntax of the named test
# arguments.  The output from this function is intended to be used as the
# argument to an "eval" statement so that the caller can create variables in
# it's scope based on the test arguments.  Additionally an array "unnamed" is
# declared which contains the arguments that could not be parsed.
#
# EXAMPLE
# 	function foo {
# 		eval "$(parse_named "$@")" || exit_error "parse_named failed"
# 		...
# 	}
# 	foo bar one=1 two='hi there' y=$(yes | head -n1) baz
#
# In this case, parse_named would effectively result in:
# 	declare one=1
# 	declare two='hi there'
# 	declare y=y
# 	declare unnamed=( bar baz )
#
# If your function isn't expecting anything other than named parameters, you can
# sanity-check ${#unnamed[@]} after calling parse_named:
# 	function foo {
# 		eval "$(parse_named "$@")" || exit_error "parse_named failed"
# 		[[ ${#unnamed[@]} -eq 0 ]] || exit_error "foo doesn't grok: ${unnamed[*]}"
# 		...
# 	}
function parse_named {
    declare x
    # bash bug prevents -a and initializer simultaneously :-/
    echo "declare -a unnamed; unnamed=();"
    for x; do
	if [[ $x == *=* ]]; then
	    printf "declare %s=%q;\n" "${x%%=*}" "${x#*=}"
	else
	    printf "unnamed+=( %q );\n" "$x"
	fi
    done
}

if [[ $PPROFILE == lspp ]]; then # XXX revisit
    # override "service" command to use run_init with strict policy
    function service {
	declare name=$1
	shift
	initcall /etc/init.d/$name "$@"
    }
fi

######################################################################
# auditd functions
######################################################################

function start_auditd {
    declare i s="starting auditd $$: can you hear me now?"
    if ! pidof auditd &>/dev/null; then
	service auditd start || return 2
    fi

    # auditd daemonizes before it is ready to receive records from the kernel.
    # make sure it's receiving before continuing.
    echo -n "start_auditd: Waiting for auditd to start"
    for ((i = 0; i < 100; i++)); do
	auditctl -r 0 >/dev/null # XXX auditctl -m "$s"
	# XXX if tail -n10 /var/log/audit/audit.log | grep -Fq "$s"; then
	if tail -n10 /var/log/audit/audit.log | grep -Fq audit_rate_limit=0; then
	    echo
	    return 0
	fi
	echo -n .
	sleep 0.1
    done

    echo
    echo "start_auditd: auditd slow starting, giving up" >&2

    return 2
}

function stop_auditd {
    declare i

    auditctl -D &>/dev/null
    service auditd stop || killall auditd
    pidof auditd &>/dev/null || return 0

    echo -n "stop_auditd: Waiting for auditd to stop"
    for ((i = 0; i < 100; i++)); do
	if ! pidof auditd &>/dev/null; then
	    echo
	    return 0
	fi
	echo -n .
	sleep 0.1
    done

    echo
    echo "stop_auditd: timed out, could not stop auditd" >&2

    return 2
}

function restart_auditd {
    # ignore the return status and messages from stop_auditd() as the
    # audit daemon may not be started yet but we don't consider that a
    # failure
    stop_auditd >& /dev/null
    start_auditd
}

function rotate_audit_logs {
    declare tmp num_logs

    if [[ -f /var/log/audit/audit.log ]]; then
        pushd /var/log/audit >/dev/null
        tmp=$(mktemp $PWD/rotating.XXXXXX) || return 2
        ln -f audit.log "$tmp" || return 2

	# Attempt to rotate using mechanism available in 1.0.10+
	echo "rotate_audit_logs: Attempting to rotate using USR1"
	if pidof auditd &>/dev/null; then
	    killall -USR1 auditd &>/dev/null
	    sleep 0.1
	fi

        # If rotation didn't work, do it manually.
        if [[ audit.log -ef $tmp ]]; then
	    echo "rotate_audit_logs: Seems that USR1 is not supported"
            stop_auditd
            num_logs=$(awk '$1=="num_logs"{print $3;exit}' $auditd_conf)
            while ((--num_logs > 0)); do
                if ((num_logs == 1)); then
		    [[ -f audit.log ]] && mv -f audit.log audit.log.1
                else
		    [[ -f audit.log.$((num_logs-1)) ]] && \
			mv -f audit.log.$((num_logs-1)) audit.log.$((num_logs))
                fi
            done
        fi

        rm -f "$tmp"
        popd >/dev/null
    fi

    pidof auditd &>/dev/null || start_auditd
}

######################################################################
# role-based utilities
######################################################################

# getcontext <user|role|type|level|cat>
function getcontext {
    declare column

    case $1 in
        user)  column=1 ;;
        role)  column=2 ;;
        type)  column=3 ;;
        level) column=4 ;;
        cat)   column=5 ;;
        *) return 2 ;;
    esac

    awk -v column=$column -F: '{print $column}' /proc/self/attr/current
}

# rolecall <newrole-args> -- <cmd>
function rolecall {
    [[ $# == 0 ]] && exit 2
    declare status

    (   # use a subshell to contain exported variables
        declare NEWROLE_ARGS CMD

        while [[ $# -gt 0 && $1 != -- ]]; do
            # this is interpreted by tcl, so use tcl quoting.
            # XXX this will break if an argument includes braces,
            # so bomb out until we have a better solution.
            if [[ $1 == *[{}]* ]]; then
                echo "braces not allowed in arguments to rolecall" >&2
                exit 111        # exit from subshell
            fi
            NEWROLE_ARGS="$NEWROLE_ARGS {$1}"
            shift
        done
        export NEWROLE_ARGS

        # drop separator -- from remaining positional params
        shift

        # build the quoted command
        while [[ $# -gt 0 ]]; do
            CMD="$CMD $(printf '%q' "$1")"
            shift
        done
        export CMD

        expect -c '
            eval spawn newrole $env(NEWROLE_ARGS) -- -c $env(CMD)
            expect -nocase {password:} {
                send "$env(PASSWD)\r"
            }
	    expect eof
	    set status [wait]
	    exit [lindex $status 3]'
    )

    # exit status from subshell
    return $?
}

# initcall <init-script-or-command> [args...]
function initcall {
    [[ $# == 0 ]] && exit 2
    declare status

    (   # use a subshell to contain exported variables
        declare RUNINIT_ARGS

        while [[ $# -gt 0 ]]; do
            # this is interpreted by tcl, so use tcl quoting.
            # XXX this will break if an argument includes braces,
            # so bomb out until we have a better solution.
            if [[ $1 == *[{}]* ]]; then
                echo "braces not allowed in arguments to initcall" >&2
                exit 111        # exit from subshell
            fi
            RUNINIT_ARGS="$RUNINIT_ARGS {$1}"
            shift
        done
        export RUNINIT_ARGS

        expect -c '
            eval spawn run_init $env(RUNINIT_ARGS)
            expect -nocase {password:} {
                send "$env(PASSWD)\r"
            }
	    expect eof
	    set status [wait]
	    exit [lindex $status 3]
	    ' </dev/null &>/dev/null 2>&1
    )

    # exit status from subshell
    return $?
}
