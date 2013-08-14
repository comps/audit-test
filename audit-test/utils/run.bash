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
# The normal usage of this script is simply "./run.bash" but additional
# usage information can be retrieved with "./run.bash --help"

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
PATH=$TOPDIR/utils:$TOPDIR/utils/bin:$PATH

source functions.bash || exit 2

#----------------------------------------------------------------------
# global variables
#----------------------------------------------------------------------

unset logging
unset opt_verbose opt_debug opt_config opt_list opt_log opt_rollup opt_timeout opt_width
echoing=true
logging=false
opt_avc=false
opt_verbose=false
opt_debug=false
opt_quiet=false
opt_config=run.conf
opt_list=false
opt_log=run.log
opt_logdir=logs
opt_rerun=false
opt_rollup=rollup.log
opt_timeout=30
opt_width=$(stty size 2>/dev/null | cut -d' ' -f2)
[[ -n $opt_width ]] || opt_width=80
header_log="run.info"

unset TESTS TNUMS
unset pass fail error total
unset auditd_orig

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
    llmsg "$@"
    colorize "$@"
}

function vmsg {
    lmsg "$@"
    if $opt_verbose || $opt_debug; then
	colorize "$@"
    fi
}

function dmsg {
    lmsg "$@"
    if $opt_debug; then
	colorize "$@"
    fi
}

function prf {
    $echoing && printf "$(colorize "$1")" "${@:2}"
    $logging && printf "$(monoize "$1")" "${@:2}" | tee -a "$opt_rollup" >>"$opt_log"
}

#----------------------------------------------------------------------
# test list manipulation
#----------------------------------------------------------------------

# run_test(char *cmdline)
# This is just the default implementation; it can be overridden in run.conf
function run_test {
    "./$@"
}

# +(char *test, char *params)
# add a test case to the list
function + {
    dmsg "Adding TESTS[${#TESTS[@]}]: $*"
    TESTS+=( "$(printf '%q ' "$@")" )
}

#----------------------------------------------------------------------
# startup/cleanup
#----------------------------------------------------------------------

trap 'cleanup &>/dev/null; close_log; exit' 0
trap 'cleanup; close_log; exit' 1 2 3 15

# early_startup runs before parsing cmdline and run.conf
function early_startup {
    # If we're running the mls policy, check that we're in the lspp_test_r role
    # Also set the protection profile if its not already set
    if sestatus 2>/dev/null | grep -q mls ; then
	    read role <<<"$(getcontext role)"
	    if [[ $role != lspp_test_r ]]; then
		die "Please run this suite as lspp_test_r"
            else
		if [[ -z $PPROFILE ]] ; then
	  	    export PPROFILE=lspp
                fi
	    fi
    elif [[ -z $PPROFILE ]]; then
	export PPROFILE=capp
    fi
}

# this can be overridden in run.conf
function startup_hook {
    true
}

# this can be overridden in run.conf
function cleanup_hook {
    true
}

# startup runs after parsing run.conf, before running tests
function startup {
    export TEST_USER=testuser
    export TEST_USER_PASSWD="2manySecre+S-$RANDOM"

    export TEST_ADMIN=testadmin
    export TEST_ADMIN_PASSWD="3manySecre+S-$RANDOM"

    dmsg "Starting up"

    # Make sure we're running as root
    if [[ $EUID != 0 ]]; then
        die "Please run this suite as root"
    fi

    # Check for password
    if [[ -z $PASSWD ]]; then
	    trap 'stty echo; exit' 1 2;
	    read -sp "Login user password: " PASSWD; echo; export PASSWD;
	    trap - 1 2;
    fi

    # Create log directory if needed
    if [[ ! -d "$opt_logdir" ]]; then
        mkdir "$opt_logdir"
    fi

    # Initialize audit configuration and make sure auditd is running
    auditd_orig=$(mktemp $auditd_conf.XXXXXX) || return 2
    cp -a "$auditd_conf" "$auditd_orig" || return 2
    write_config -s "$auditd_conf" \
	log_format=RAW \
	flush=SYNC \
	max_log_file_action=ROTATE \
	    || return 2
    # remove the configuration for the audit dispatcher
    write_config -r "$auditd_conf" dispatcher DISP_qos || return 2

    if [[ $PPROFILE == lspp ]] ; then
        chcon system_u:object_r:auditd_etc_t:s15:c0.c1023 $auditd_orig
        chcon system_u:object_r:auditd_etc_t:s15:c0.c1023 $auditd_conf
    fi

    start_auditd >/dev/null || die
    killall -HUP auditd	# reload config when auditd was already running

    # Add the test user which is used for unprivileged tests
    userdel -r "$TEST_USER" &>/dev/null
    groupdel "$TEST_USER" &>/dev/null
    dmsg "Adding group $TEST_USER"
    groupadd "$TEST_USER" || die
    dmsg "Adding user $TEST_USER"
    useradd -g "$TEST_USER" -G wheel -m "$TEST_USER" || die
    echo "$TEST_USER_PASSWD" | passwd --stdin $TEST_USER >/dev/null
    faillock --user "$TEST_USER" --reset

    # Add the test user which is in sysadm_r
    userdel -r "$TEST_ADMIN" &>/dev/null
    groupdel "$TEST_ADMIN" &>/dev/null
    dmsg "Adding group $TEST_ADMIN"
    groupadd "$TEST_ADMIN" || die
    dmsg "Adding user $TEST_ADMIN"
    if [[ $PPROFILE == lspp ]] ; then
        useradd -Z sysadm_u -g "$TEST_ADMIN" -G wheel -m "$TEST_ADMIN" || die
    else
        useradd -g "$TEST_ADMIN" -G wheel -m "$TEST_ADMIN" || die
    fi
    echo "$TEST_ADMIN_PASSWD" | passwd --stdin $TEST_ADMIN >/dev/null
    faillock --user "$TEST_ADMIN" --reset

    startup_hook
}

function cleanup {

    cleanup_hook

    # Remove the test user
    # XXX use prepend_cleanup in startup
    if [[ -n $TEST_USER ]]; then
	# Remove the test user
	dmsg "Killing all processes for $TEST_USER"
	killall -9 -u "$TEST_USER"
	dmsg "Removing user $TEST_USER"
	userdel -r "$TEST_USER" &>/dev/null
	dmsg "Removing group $TEST_USER"
	groupdel "$TEST_USER" &>/dev/null
    fi

    # Remove the test admin user
    # XXX use prepend_cleanup in startup
    if [[ -n $TEST_ADMIN ]]; then
        # Remove the test user
        dmsg "Killing all processes for $TEST_ADMIN"
        killall -9 -u "$TEST_ADMIN"
        dmsg "Removing user $TEST_ADMIN"
        userdel -r "$TEST_ADMIN" &>/dev/null
        dmsg "Removing group $TEST_ADMIN"
        groupdel "$TEST_ADMIN" &>/dev/null
        if [[ $PPROFILE == lspp ]] ; then
            semanage login -d "$TEST_ADMIN"
        fi
    fi

    # Restore the original auditd configuration
    # XXX use prepend_cleanup in startup
    if [[ -s $auditd_orig ]]; then
        mv "$auditd_orig" "$auditd_conf"
	killall -HUP auditd
    fi
    rm -f "$auditd_orig"
}

function prepend_cleanup {
    eval "function cleanup {
	$*
	$(type cleanup | sed '1,3d;$d')
    }"
}

function append_cleanup {
    eval "function cleanup {
	$(type cleanup | sed '1,3d;$d')
	$*
    }"
}

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
    -g --generate     Generate run.log and rollup.log from $opt_logdir
       --header       Don't run anything, just create and output the log header
    -l --log=FILE     Output to a log other than run.log
    -r --rerun        Run only those tests that did not pass
       --rollup=FILE  Output to a rollup other than rollup.log
    -t --timeout=SEC  Seconds to wait for a test to timeout, default 30
    -o --logdir=DIR   Output directory of per test logs
    -w --width=COLS   Set COLS output width instead of auto-detect
    -h --help         Show this help

Output modes:
    (default)         Pass/fail/error status only
       --list         List the available tests
    -a --avc          Print AVCs after testexecution and audit2allow rules
    -v --verbose      Copious output on fail or error
    -d --debug        Copious output always
    -q --quiet        Suppress error/fail message
       --nocolor      Don't use ANSI color sequences

EOF
}

function parse_cmdline {
    declare args conf x

    # Use /usr/bin/getopt which supports GNU-style long options
    args=$(getopt -o adf:ghl:qro:vw: \
        --long config:,avc,debug,generate,help,header,list,log:,logdir:,quiet,rerun,rollup:,nocolor,verbose,width: \
        -n "$0" -- "$@") || die
    eval set -- "$args"

    while true; do
        case $1 in
            -a|--avc) opt_avc=true; shift ;;
            -d|--debug) opt_debug=true; opt_verbose=true; shift ;;
            -f|--config) opt_config=$2; shift 2 ;;
            -g|--generate) logging=true; generate_logs; exit 0 ;;
            -h|--help) usage; exit 0 ;;
	    --header) show_header; exit 0 ;;
            --list) opt_list=true; shift ;;
            -l|--log) opt_log=$2; shift 2 ;;
	    -q|--quiet) opt_quiet=true; shift ;;
            --rollup) opt_rollup=$2; shift 2 ;;
            -r|--rerun) opt_rerun=true; shift ;;
            -t|--timeout) opt_timeout=$2; shift 2 ;;
            -o|--logdir) opt_logdir=$2; shift 2 ;;
            --nocolor) colorize() { monoize "$@"; }; shift ;;
            -v|--verbose) opt_verbose=true; shift ;;
	    -w|--width) opt_width=$2; shift 2 ;;
            --) shift ; break ;;
            *) die "failed to process cmdline args" ;;
        esac
    done

    # Load the config
    dmsg "Loading config from $opt_config"
    conf="$(<$opt_config)
          true"
    eval -- "$conf" || die "Error reading config file: $opt_config"

    if [[ -n $* ]]; then
	# Additional cmdline indicates tests to run
        dmsg "Filtering TESTS by cmdline"
        while [[ -n $1 ]]; do
	    if [[ $1 == *[!0-9]* ]]; then
		# add by string
		for ((x = 0; x < ${#TESTS[@]}; x++)); do
		    # match on "words", allow globbing within a word
		    if [[ " ${TESTS[x]} " == *[\ =]$1" "* ]] && rerun_test $x; then
			dmsg "  $1 matches [$x] ${TESTS[x]}"
			TNUMS[x]=$x
		    fi
		done
	    else
		# add by number
		if [ $1 -lt ${#TESTS[@]} ] && rerun_test $1; then
		    dmsg "  [$1] ${TESTS[$1]}"
		    TNUMS[$1]=$1
		fi
	    fi
	    shift
	done
	# drop unset elements from TNUMS array
	TNUMS=( ${TNUMS[@]} )
    else
	# Run all the tests
	for ((x = 0; x < ${#TESTS[@]}; x++)); do
	    # match on "words", allow globbing within a word
	    rerun_test $x && TNUMS[$x]=$x
	done
    fi
    [[ ${#TNUMS[@]} -gt 0 ]] || die "no matching tests"

    if $opt_list; then
	declare TESTNUM
	for TESTNUM in "${TNUMS[@]}"; do
	rerun_test $TESTNUM || continue
	    eval "set -- ${TESTS[TESTNUM]}"
	    nolog show_test "$@"
	    echo
	done
	exit 0
    fi

    # Open the logs before running the tests
    open_log
}

function show_header {
    # Create log directory if needed
    if [[ ! -d "$opt_logdir" ]]; then
        mkdir "$opt_logdir"
    fi

    # Create header file
    {
        echo
        printf "%-32s %s\n" Started: "$(date)"
        printf "%-32s %s\n" Kernel: "$(uname -r)"
        printf "%-32s %s\n" Architecture: "$(uname -m)"
        printf "%-32s %s\n" Mode: "${MODE:-(native)}"
        printf "%-32s %s\n" Hostname: "$(uname -n)"
        printf "%-32s %s\n" Profile: "$PPROFILE"
        printf "%-32s %s\n" "selinux-policy version:" "$(rpm -q selinux-policy)"
        if [[ $PPROFILE == lspp ]] ; then
          printf "%-32s %s\n" "lspp_test policy version:" "$(semodule -l | grep lspp_test | awk '{print $2}')"
        fi
        printf "\n%s\n" "$(sestatus)"
        echo
    } | tee $opt_logdir/$header_log
}

function fmt_test {
    declare i s indent width

    # longest result is " ERROR (255)" == 12 chars
    width=$((opt_width - 12))

    # Each time through this loop, display as much of the line as fits in $width,
    # then remove displayed args from positional parameter list.
    i=$#
    while (( i > 0 )); do
	s="$indent${*:1:i}"
	# if the last arg is still too big, print it anyway
	if (( ${#s} <= width || i == 1 )); then
	    prf "%-${width}s " "$s"
	    # try to be smart about the indent for lines 2..N.
	    if [[ -z $indent ]]; then
		indent='        '          # max indent = 8
		indent=${indent:0:${#1}+1} # min indent = len($1)+1
	    fi
	    shift $i
	    i=$#
	    # need a newline if we have more args to display
	    (( i > 0 )) && prf '\n'
	else
	    (( i-- ))
	fi
    done
}

function show_test {
    fmt_test "[$TESTNUM]" "$@"
}

function noecho {
    declare echoing=false
    "$@"
}

function nolog {
    declare logging=false
    "$@"
}

function generate_logs {
    declare pass fail error

    # clear run and rollup logs
    echo -n > $opt_log
    echo -n > $opt_rollup

    # add header to run log if exists
    [ -f $opt_logdir/$header_log ] && cat $opt_logdir/$header_log > $opt_log

    # create total run log
    for log in $(ls $opt_logdir/$opt_log.* | sed 's/\(.*\)\.\(.*\)/\1 \2/g' | sort -k2 -n | tr ' ' '.'); do
        cat $log >> $opt_log
        echo >> $opt_log
    done

    # create total rollup log
    for log in $(ls $opt_logdir/$opt_rollup.* | sed 's/\(.*\)\.\(.*\)/\1 \2/g' | sort -k2 -n | tr ' ' '.'); do
        cat $log | sed '1,/--------/d' >> $opt_rollup
    done

    pass=$(grep "PASS" $opt_rollup | wc -l)
    fail=$(grep "FAIL" $opt_rollup | wc -l)
    error=$(grep "ERROR" $opt_rollup | wc -l)
    (( total = pass + fail + error ))
    llmsg
    prf "%4d pass (%d%%)\n" $pass $((pass * 100 / total))
    prf "%4d fail (%d%%)\n" $fail $((fail * 100 / total))
    prf "%4d error (%d%%)\n" $error $((error * 100 / total))
    prf "%s\n" "------------------"
    prf "%4d total\n" $total
}

function rerun_test {
    # if not in rerun mode - always run
    $opt_rerun || return 0

    # run test if it did not run yet
    [ ! -f "$opt_logdir/rollup.log.$1" ] && return 0

    # if test passed do not run
    grep -q "\[[0-9]\+\].*PASS[[:space:]]*$" $opt_logdir/rollup.log.$1 && return 1

    return 0
}

function run_tests {
    declare TESTNUM output status hee s log stats header
    declare begin_output="<blue>--- begin output -----------------------------------------------------------"
    declare end_output="<blue>--- end output -------------------------------------------------------------"

    nolog prf "%-$((opt_width-7))s %s\n" "Testcase" "Result"
    nolog prf "%-$((opt_width-7))s %s\n" "--------" "------"

    if $opt_debug; then
	hee=/dev/stderr
    else
	hee=/dev/null
    fi

    for TESTNUM in "${TNUMS[@]}"; do
	noecho prf "%-$((opt_width-7))s %s\n" "Testcase" "Result"
	noecho prf "%-$((opt_width-7))s %s\n" "--------" "------"

	eval "set -- ${TESTS[TESTNUM]}"

	if $opt_debug; then
	    nolog show_test "$@"
	    nolog msg "<blue>DEBUG"
	    nolog msg "$begin_output"
	else
	    show_test "$@"
	fi

	# get current time
	stime=$(date +'%H:%M:%S')
	output=$(
# note that putting run_test in the background results in no tty for pam tests
	    ( exec > >(tee $hee) 2>&1; run_test "$@"; ) # &
#	    pid=$!
# opt_timeout is disabled for now due to a bash bug.  If the timeout is put into
# the background and $pid exits before wait is called, the wait will fail
# because bash claims $pid is not a child of this shell.  See
# http://lists.gnu.org/archive/html/bug-bash/2005-12/msg00025.html
#	    if [[ $opt_timeout > 0 ]]; then
#		( sleep $opt_timeout; kill $pid; ) &>/dev/null &
#	    fi
#	    wait $pid
	) 2>&1
	status=$?
	etime=$(date +'%H:%M:%S')

	if $opt_debug; then
	    nolog msg "$end_output"
	    show_test "$@"
	fi

	if [[ $status == 0 ]]; then
	    prf "<green>%11s\n" "PASS "
	    (( pass++ ))
	    if $opt_verbose; then
		s=$(sed -n 's/^exit_pass:/       /p' <<<"$output")
		[[ -n $s ]] && prf "%s\n" "$s"
	    fi
	    lmsg "$begin_output"
	    lmsg "$output"
	    lmsg "$end_output"
	    lmsg
	else
	    if [[ $status == 1 ]]; then
		prf "<yellow>%11s\n" "FAIL "
		(( fail++ ))
		if ! $opt_quiet; then
		    s=$(sed -n 's/^exit_fail:/       /p' <<<"$output")
		    [[ -n $s ]] && prf "%s\n" "$s"
		fi
	    else
		prf "<red>%11s\n" "ERROR ($status)"
		(( error++ ))
		if ! $opt_quiet; then
		    s=$(sed -n 's/^exit_error:/       /p' <<<"$output")
		    [[ -n $s ]] && prf "%s\n" "$s"
		fi
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

	# print AVCs if requested
	if $opt_avc; then
	    msg "<blue>-- Test execution AVC records ----------------------------------------------"
	    msg "$(ausearch -ts $stime -te $etime -m avc)"
	    msg "<blue>-- audit2allow -------------------------------------------------------------"
	    msg "$(ausearch -ts $stime -te $etime -m avc | audit2allow)"
	fi

	# copy header to run and rollup log
	echo "$header" >> $opt_logdir/$opt_log.$TESTNUM
	echo >> $opt_logdir/$opt_log.$TESTNUM
	echo "$header" >> $opt_logdir/$opt_rollup.$TESTNUM
	echo >> $opt_logdir/$opt_rollup.$TESTNUM

	# copy test output to own log file
	cp -f $opt_log $opt_logdir/$opt_log.$TESTNUM
	sed -i '/./,$!d' $opt_logdir/$opt_log.$TESTNUM
	cp -f $opt_rollup $opt_logdir/$opt_rollup.$TESTNUM
	sed -i '/./,$!d' $opt_logdir/$opt_rollup.$TESTNUM

	# clear log and rollup
	echo -n > $opt_log
	echo -n > $opt_rollup
    done

    # create current stats
    (( total = pass + fail + error ))
    nolog msg
    nolog prf "%4d pass (%d%%)\n" $pass $((pass * 100 / total))
    nolog prf "%4d fail (%d%%)\n" $fail $((fail * 100 / total))
    nolog prf "%4d error (%d%%)\n" $error $((error * 100 / total))
    nolog prf "%s\n" "------------------"
    nolog prf "%4d total\n" $total

    # create silently run and rollup logs
    noecho generate_logs

    return 0
}

early_startup
parse_cmdline "$@"
startup || die "startup failed"
run_tests
exit $?
