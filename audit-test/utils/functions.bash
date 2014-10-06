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

# xtables_empty - based on xtables-save input, makes a clean ruleset, ready
#                 to be used with xtables-restore
#
# INPUT:  xtables-save format, preferably unmodified
# OUTPUT: xtables-restore format, ready to be used
#
# DESCRIPTION:
#   The problem with xtables -F ; xtables -X ; xtables -P ... ACCEPT is that
#   they're related only to the `filter' table by default - they would need to
#   be called a lot more times to clean other used tables like `raw', `mangle',
#   `nat' and `security', finding out somehow whether any of them is used first.
#
#   A simple xtables-restore solution, restoring empty ruleset, is also not easy
#   - if you specify ie. only the `filter' table, no other table gets touched.
#   If you specify all possible tables, all modules related to those tables get
#   loaded, even if they weren't originally loaded (ie. iptable_* modules).
#
#   The solution is therefore to parse xtables-save output to find out which
#   tables are used and generate empty ruleset to zero them.
#   This solution can be generic enough to work for iptables, ip6tables,
#   ebtables, arptables and any other tables there might be.
function xtables_empty {
    # grep:
    #   - currently loaded table names
    #   - only predefined chains (no user chains)
    #   - include COMMIT statements for each table
    # sed:
    #   - replace DROP default policies by ACCEPT
    #   - zero packet and byte counters
    grep -e '^\*' -e '^:[^ ]* [^-]' -e '^COMMIT$' | sed 's/DROP/ACCEPT/ ; s/\[[0-9]*:[0-9]*\]/\[0:0\]/'
}

# tstsvr_cleanup - cleanup the network server at a specified host
#
# DESCRIPTION:
#   This script can be executed after each networking test as a sanity cleanup,
#   to either kill any unfinished lblnet_tst_server instances spawned by xinetd,
#   or to ensure that no remaining instances are frozen even when expres=success.
function tstsvr_cleanup {
    nc -w 3 "$1" 4009 </dev/null
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

# backup - backup files, with automatic restore on exit
#
# usage: backup file1 file2 dir3 ...
function backup {
    declare f b
    for f in "$@"; do
    # if the path/file does not exist - skip backup
    if [ -e "$f" ]; then
        b=$(mktemp "$f.XXXXXX") || exit_error
        cp -a "$f" "$b" || exit_error
        prepend_cleanup "mv -f '$b' '$f'"
    fi
    done
}

######################################################################
# testing helpers
######################################################################

# evaluate a syscall wrapper result
#
# read expected result ("pass"/"fail") from $1 and expected numeric errno
# from $2 (in case of "fail") and execute the rest of the arguments, catching
# stderr and expecting '<result> <errno> <pid>' output
# - if no such output is found, exit_error
# - if <result> doesn't match expected result, exit_fail
# - if <result> is "fail" and <errno> doesn't match expected errno, exit_fail
# - else do nothing (ie. allowing the test case to exit_pass)
#
# errno can also be given as a comma-separated list of possible values
#
# exports a bash array called EVAL_SYSCALL_RESULT with 3 members, mapped
# directly to the three values returned by the syscall (result, errno, pid)
#
# usage: eval_syscall <expected_result> <expected_errno> <cmd> [args]
eval_syscall()
{
    local res= exp_res="$1" errno= exp_errno="$2"
    local i= rc= ret= tmp= tmpfifo= junk=

    [ $# -lt 3 ] && exit_error "$FUNCNAME: not enough arguments"
    shift 2

    # translate pass/fail into 0/1
    case "$exp_res" in
        pass)  exp_res=0 ;;
        fail)  exp_res=1 ;;
           *)  exit_error "$FUNCNAME: invalid expected result" ;;
    esac

    # execute the command, catch stderr
    # NOTE: no reasonably safe way to do this without tempfile in bash,
    #       we need to preserve this shell since <cmd> may be another
    #       shell function (ie. another wrapping one) calling exit_*
    # NOTE: can't use pipes and stderr/out swap due to the need of { },
    #       breaking *pend_cleanup redefinitions in nested functions
    # NOTE: "Process substitution" is *unsafe* here as bash waits only
    #       for the first process to finish, not for the "tee", resulting
    #       in grep getting empty file (before tee is able to fill it),
    #       therefore we use named pipes as a primitive "mutex" lock
    tmpfifo=$(mktemp -u) || exit_error
    prepend_cleanup "rm -f \"$tmpfifo\""
    mkfifo "$tmpfifo" || exit_error
    tmp=$(mktemp) || exit_error
    prepend_cleanup "rm -f \"$tmp\""

    "$@" 2> >(tee "$tmp" 1>&2; echo >"$tmpfifo")
    rc=$?
    read junk <"$tmpfifo"  # wait for tee to finish

    # try to find something that looks like a syscall wrapper output,
    # use only first match (no more than one should be present anyway)
    # NOTE: we can't just use all of stderr, bash -x output is there
    ret=$(grep -m1 '^[0-9] [0-9]\+ [0-9]\+$' "$tmp") || exit_error \
        "$FUNCNAME: no matching syscall wrapper result found"

    read res errno junk <<<"$ret"
    declare -g -a EVAL_SYSCALL_RESULT=("$res" "$errno" "$junk")

    # compare result
    [ "$res" -eq "$exp_res" ] || \
        exit_fail "syscall result $res doesn't match expected value $exp_res"

    # sanity - wrapper return code (0/1) should equal syscall result (0/1)
    [ "$rc" -eq "$res" ] || \
        exit_error "syscall result $res doesn't match wrapper rc $rc"

    # if fail, compare errno
    if [ "$res" -eq 1 ]; then
        local IFS=','
        for i in $exp_errno; do [ "$i" -eq "$errno" ] && break; done
        [ "$i" -eq "$errno" ] || \
            exit_fail "syscall errno $errno doesn't match expected value $exp_errno"
    fi

    # else do nothing
    return 0
}

# Call remote executable
#
# On NS lblnet_test_svr is able to execute any executable in remote_call
# directory in utils/network-server. It might be either script or a binary
# executable. This function call lblnet_tst_server to do such remote
# execution. First parameter is the name of executable and the second
# (optional) parameter is data to be given to executable. Please see
# remote_call control message documentation for a format of such data.
#
# usage: remote_call <executable> [data]
#
function remote_call {

    local executable="$1"
    local data="$2"

    [ -z "$executable" ] && exit_error "Missing executable"
    [ -z "$LBLNET_SVR_IPV4" ] && exit_erro "LBLNET_SVR_IPV4 is not exported"

    echo "---- START remote_call ----"

    echo "Executable = $executable"
    if [ -z "$data" ]; then
        echo "Data = (no data)"
    else
        echo "Data = $data"
    fi
    echo ""

    /usr/bin/nc -v $LBLNET_SVR_IPV4 4000 <<< "remote_call:$executable,$data;"

    echo "---- END remote_call ----"

    tstsvr_cleanup $LBLNET_SVR_IPV4
}

# Check if machine is in FIPS mode
#
# return 0 if machine in FIPS mode, 1 otherwise
function is_fips {
    [ -e "/etc/system-fips" ] || return 1
    [ $(sysctl -n crypto.fips_enabled) -eq 0 ] && return 1

    return 0
}

# check syscall relevancy to currently running system
#
# returns 0 if a syscall is relevant, 1 otherwise
sc_is_relevant()
{
	case " $SCREL_SYSCALLS " in *" $1 "*) return 0;; esac
	return 1
}

######################################################################
# service functions
######################################################################

function start_service {

    local service="sbin/${1}"

    case $1 in
	"ipsec" )
	    service="ipsec/pluto"
	    ;;
    esac

    # systemctl will block service if started to often
    # clear the failed state of the service
    systemctl reset-failed $1

    if ! pgrep -f $service &>/dev/null; then
	if [ "$DISTRO" = "SUSE" ]; then
	    rc${1} start || return 2
	else
	    # XXX: fd 63 is left open by something, causing the tests to hang
	    service $1 start 63>/dev/null || return 2
	fi
    fi

    echo -n "start_service: Waiting for $1 to start"
    if ! wait_for_cmd "pgrep -f $service"; then
	echo "start_service: timed out, could not start $1" >&2
	return 2
    fi

    # Services specifics.
    case $1 in
	"auditd" )
	    [ "$DISTRO" = "SUSE" ] && ( auditctl -e 1 || return 2 )

	    local log_file=${2:-"$audit_log"}

            # auditd daemonizes before it is ready to receive records from the kernel.
            # make sure it's receiving before continuing.
            if ! wait_for_cmd "auditctl -r 0 >/dev/null; tail -n10 $log_file | grep -Fq audit_rate_limit=0"; then
		echo "start_service: auditd slow starting, giving up" >&2
		return 2
	    fi
	    ;;

	"ipsec")
	    if ! ipsec status >/dev/null 2>&1; then
                echo "start_service: cannot start ipsec" >&2
	    else
		ip xfrm policy
	    fi
	    ;;

        "sshd")
            # make sure sshd is listening
            if ! wait_for_cmd "ss -tnlp | grep -q '\"sshd\"'"; then
                echo "start_service: cannot start sshd" >&2
                return 2
            fi
            # make sure a TCP client can connect to it
            local ssh_port=$(ss -4 -tnlp | grep '"sshd"' | awk '{print $4}' | sed -rn '1s/.+:([0-9]+)$/\1/p')
            if ! wait_for_cmd "echo -ne \'\\\004\' | nc -w 1 127.0.0.1 \"$ssh_port\""; then
                echo "start_service: sshd still refuses connection requests" >&2
                return 2
            fi
            ;;
        "cups")
            # make sure cups is listening on socket
            if ! wait_for_cmd "lsof -c cupsd -a /var/run/cups/cups.sock"; then
                echo "start_service: cannot start cups" >&2
                return 2
            fi
            ;;
    esac

    return 0
}

function stop_service {

    local service="sbin/${1}"

    # Services specifics.
    case $1 in
	"auditd" ) auditctl -D &>/dev/null ;;
	"ipsec" ) service="ipsec/pluto" ;;
    esac

    if [ "$DISTRO" = "SUSE" ]; then
	rc${1} stop || killall $1
    else
	service $1 stop || killall $1
    fi

    echo -n "stop_service: Waiting for $1 to stop"
    # wait until pgrep starts failing (process exited)
    if ! wait_for_cmd "pgrep -f $service" not 0; then
        echo "start_service: timed out, could not stop $1" >&2
        return 2
    fi

    return 0
}

function restart_service {
    stop_service $1
    start_service $1
}

######################################################################
# auditd functions
######################################################################

function rotate_audit_logs {
    declare tmp num_logs

    if [[ -f "$audit_log" ]]; then
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
            stop_service auditd
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

    pidof auditd &>/dev/null || start_service auditd
}

# print block size of audit log (used for augrok --seek)
function get_audit_mark {
    echo "$(stat -c %s $audit_log)"
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
            eval spawn newrole $env(NEWROLE_ARGS) -- -c \"$env(CMD)\"
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

######################################################################
# synchronization
######################################################################

# these functions can be used to block until a certain condition is met

# feel free to override these variables in your test
# - timeouts
WAIT_FOR_SLEEP=0.1  # how long (in sec) to wait between iterations
WAIT_FOR_MAX=100    # for how many iterations
                    # (100*0.1 = ~10sec, ignoring the command exec time)

# wait for a command to return a specified value
# executes $1 until it returns 0
# if $2 is specified, wait for $1 to return code $2
# if $2 is "not", use $3 instead and negate the condition
# (eg. wait until cmd stops returning $3)
#
# ie.
#  wait_for_cmd "cmd"        # wait for rc to be 0
#  wait_for_cmd "cmd" 2      # wait for rc to be 2
#  wait_for_cmd "cmd" not 1  # wait for rc to be 0 or 2-255
#
# use case - virtually anything, ie.
#  # wait for file to exist
#  "[ -e $file ]"
#  # wait for a file to be logrotated (inode changed && exists)
#  "[ \"\$(stat -c %i $file)\" != 123456 -a \$? -eq 0 ]"
#  # wait for anything to start listening on tcp port 1234
#  "ss -tnl | grep -q 1234"
wait_for_cmd()
{
    local i="$WAIT_FOR_MAX"
    local cmd="$1" rc="$2" notrc="$3" op='='
    [ "$rc" ] || rc=0
    if [ "$rc" = "not" ]; then
        [ "$notrc" ] || return 1
        rc="$notrc"; op='!='
    fi
    for ((;i>0;i--)); do
        eval "$cmd" &>/dev/null
        [ "$?" "$op" "$rc" ] && return 0
        sleep "$WAIT_FOR_SLEEP"
    done
    return 1
}

# wait for pid $1 to have at least 1 child (fork done) and return its pid
#
# use case - simple command wrapper, which does something and spawns a child,
# doing execve in it, like "initwrap" from syscalls-ns -- note that this
# function waits only for the fork, not the execve, ie.
#   ./initwrap nc -l 1234 &
#   ncpid=$(wait_for_child $!) || exit_error "timed out"
wait_for_child()
{
    local i="$WAIT_FOR_MAX"
    local pid="$1" child=
    for ((;i>0;i--)); do
        child=$(pgrep -oP "$pid") && { echo "$child"; return 0; }
        sleep "$WAIT_FOR_SLEEP"
    done
    return 1
}

# wait for pid $1 to have name $2 (ie. after execve)
# if $3 contains 'full', wait for full cmdline spec instead of binary name
#
# use case - setup binaries like unshare(1), which do something (ie. to
# the environment) and call execve -- useful if you need to wait for that
# "something" to be done (ie. namespace switch) prior to other work, ie.
#   unshare --mount nc -l 1234 &
#   wait_for_pname $! nc || exit_error "timed out"
wait_for_pname()
{
    local i="$WAIT_FOR_MAX"
    local pid="$1" pname="$2" full="$3" pexe=
    [ "$pid" -a "$pname" ] || return 1
    [ "$full" = "full" ] && full='' || full='-c'
    for ((;i>0;i--)); do
        pexe=$(ps $full -o cmd --no-headers -p "$pid")
        [ "$pexe" ] || return 0  # already ended
        [ "$pexe" = "$pname" ] && return 0
        sleep "$WAIT_FOR_SLEEP"
    done
    return 1
}

# wait for pid $1 to have a child with name $2 and return its pid
# if $3 contains 'full', wait for full cmdline spec
# (combines wait_for_pname and wait_for_child functions together)
#
# use case - waiting for bash function, spawned in background, to spawn
# a particular command, ie.
#   func() { nc 1.1.1.1 2222 <<<"woohoo"; sleep 1; nc -l 1234; }
#   func &
#   ncpid=$(wait_for_child_pname $! "nc -l 1234" full) || exit_error "timed out"
wait_for_child_pname()
{
    local i="$WAIT_FOR_MAX"
    local pid="$1" pname="$2" full="$3" children= c= pexe=
    [ "$full" = "full" ] && full='' || full='-c'
    for ((;i>0;i--)); do
        ps -o pid --no-headers -p "$pid" >/dev/null || return 1  # parent ended
        children=$(pgrep -P "$pid")
        for c in $children; do
            pexe=$(ps $full -o cmd --no-headers -p "$c")
            [ "$pexe" ] || continue  # already ended
            [ "$pexe" = "$pname" ] && { echo "$c"; return 0; }
        done
        sleep "$WAIT_FOR_SLEEP"
    done
    return 1
}

######################################################################
# sssd functions
######################################################################

# disable sssd seeding from /dev/random
function sssd_disable_strong_rng {
    local SSSDCONF="/etc/sysconfig/sssd"

    backup $SSSDCONF
    sed -i 's/^\(NSRAND.*\)/# \1/' $SSSDCONF
    systemctl restart sssd
}
