#!/bin/bash
###############################################################################
# (c) Copyright Hewlett-Packard Development Company, L.P., 2006
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
###############################################################################

source testcase.bash || exit 2

set -x

######################################################################
# global variables
######################################################################

# NOTE: these are not truly global since this file is sourced from inside
#       run_test(), so declare them with "declare"

# test utility args
declare op dirname source target flag setcontext zone dst msg value

# audit record fields
declare log_mark syscall success pid auid exitval name
declare uid=0 euid=0 suid=0 fsuid=0
declare gid=0 egid=0 sgid=0 fsgid=0

# audit record fields - mac only
declare subj obj
declare -a opid

######################################################################
# common functions
######################################################################

# usage: check_result <exp result> <syscall result> <syscall exit> <exp error>
function check_result {
    declare suc=$1 res=$2 ext=$3 err=$4

    if [[ -n $err ]]; then
        err=$(get_error_code $err)
    fi

    # success/fail set in common startup, so we can assume only two cases
    case $suc in
        success) [[ $res != 0 ]] && exit_error "unexpected test result" ;;
        fail) [[ $res != 1 ]] && exit_error "unexpected test result"
             [[ $ext != $err ]] && exit_error "unexpected test error"
             # audit represents errors as negative numbers; fix global field value
             exitval=-$ext ;;
    esac
}

# usage: get_error_code <error_name, e.g. EPERM>
function get_error_code {
    gcc -E -dM /usr/include/asm-generic/errno-base.h | grep $1 | awk '{print $3}'
}

# usage: get_ipc_op <e.g. msgctl>
function get_ipc_op {
    gcc -E -dM /usr/include/asm-generic/ipc.h | grep -i $1 | awk '{print $3}'
}

# usage: get_socketcall_op <e.g. bind>
function get_socketcall_op {
    gcc -E -dM /usr/include/linux/net.h | grep -i SYS_$1 | awk '{print $3}'
}

function setpid {
    "$@" &
    pid=$!
    wait $pid
}

######################################################################
# custom test functions
######################################################################
function test_su_default {
    declare testargs testuser

    # setup args for test operation
    if [[ $# == 0 ]]; then
	# don't quote so empty args disappear
	set -- $op $dirname $source $target $flag
    fi

    # do the test
    [[ -z $user ]] && exit_error "test \$user undefined"
    if [[ $user == super ]]; then
	read testres exitval pid <<<"$(do_$syscall "$@")"
    else
	if [[ $user == "test" ]]; then
	    testuser=$TEST_USER
	else
	    testuser=$user
	fi
	testargs=$(printf '%q ' "$@")

	# use single quotes so $$ doesn't expand early
	read uid euid suid fsuid gid egid sgid fsgid \
	    <<<"$(/bin/su - $testuser -c 'ps --no-headers -p $$ -o uid,euid,suid,fsuid,gid,egid,sgid,fsgid')"
	read testres exitval pid \
	    <<<"$(/bin/su - $testuser -c "$(which do_$syscall) $testargs")"
    fi
}

function test_su_fork {
    declare testuser

    [[ -z $user ]] && exit_error "test \$user undefined"
    if [[ $user == super ]]; then
	saved=$(ulimit -u)
	prepend_cleanup "ulimit -u $saved"
	ulimit -u 2
	read testres exitval pid <<<"$(do_$syscall)"
    else
	if [[ $user == "test" ]]; then
	    testuser=$TEST_USER
	else
	    testuser=$user
	fi

	# use single quotes so $$ doesn't expand early
	read uid euid suid fsuid gid egid sgid fsgid \
	    <<<"$(/bin/su - $testuser -c 'ps --no-headers -p $$ -o uid,euid,suid,fsuid,gid,egid,sgid,fsgid')"
	read testres exitval pid \
	    <<<"$(/bin/su - $testuser -c "ulimit -u 2 ; $(which do_$syscall)")"
    fi
}

function test_su_fsgid_set {
    test_su_default $flag
    # audit record reflects fsgid _after_ syscall
    fsgid=$flag
}

function test_su_fsuid_set {
    test_su_default $flag
    # audit record reflects fsuid _after_ syscall
    fsuid=$flag
}

function test_su_msg_send {
    test_su_default $op $target $flag "$msg"
}

function test_su_setxattr {
    test_su_default $target $flag $value
}

function test_su_time_zone {
    test_su_default $flag $zone $dst
}

function test_runcon_default {
    read testres exitval pid \
	<<<"$(runcon $subj -- do_$syscall $op $dirname $source $target $flag $setcontext)"
}

function test_runcon_kill_pgrp {
    read testres exitval pid <<<"$(runcon $subj -- do_$syscall $target $flag group)"
}

function test_runcon_msg_send {
    read testres exitval pid <<<"$(runcon $subj -- do_$syscall $op $target $flag "$msg")"
}

######################################################################
# common augrok functions
######################################################################

function augrok_default {
    augrok --seek=$log_mark -m1 type==SYSCALL \
        syscall=$syscall success=$success pid=$pid auid=$(</proc/self/loginuid) \
        uid=$uid euid=$euid suid=$suid fsuid=$fsuid \
        gid=$gid egid=$egid sgid=$sgid fsgid=$fsgid exit=$exitval \
        "$@"
}

function augrok_name {
    augrok_default name=$name
}

function augrok_inode {
    augrok_default inode=$inode
}

function augrok_op {
    declare a0

    [[ $(type -t get_${syscall}_op) == function ]] || \
	exit_error "get_${syscall}_op function does not exist"
    a0=$(printf "%x" $(get_${syscall}_op $op))

    augrok_default a0=$a0
}

# used for 32-bit shmat() success testcases:
# don't check the exit value because the arch code gives 0 to the audit hook
function augrok_op_no_exit {
    declare a0

    [[ $(type -t get_${syscall}_op) == function ]] || \
	exit_error "get_${syscall}_op function does not exist"
    a0=$(printf "%x" $(get_${syscall}_op $op))

    augrok --seek=$log_mark -m1 type==SYSCALL \
        syscall=$syscall success=$success pid=$pid auid=$(</proc/self/loginuid) \
        uid=$uid euid=$euid suid=$suid fsuid=$fsuid \
        gid=$gid egid=$egid sgid=$sgid fsgid=$fsgid \
	a0=$a0
}

function augrok_mls_label {
    augrok_default subj=$subj obj=$obj
}

function augrok_mls_name_label {
    augrok_default subj=$subj obj#a=$obj name#a=$name
}

function augrok_mls_inode_label {
    augrok_default subj=$subj obj#a=$obj inode#a=$inode
}

function augrok_mls_op_label {
    declare a0

    [[ $(type -t get_${syscall}_op) == function ]] || \
	exit_error "get_${syscall}_op function does not exist"
    a0=$(printf "%x" $(get_${syscall}_op $op))
    
    augrok_default a0=$a0 subj=$subj obj=$obj
}

# used for 32-bit shmat() success testcases:
# don't check the exit value because the arch code gives 0 to the audit hook
function augrok_mls_op_label_no_exit {
    declare a0

    [[ $(type -t get_${syscall}_op) == function ]] || \
	exit_error "get_${syscall}_op function does not exist"
    a0=$(printf "%x" $(get_${syscall}_op $op))

    augrok --seek=$log_mark -m1 type==SYSCALL \
        syscall=$syscall success=$success pid=$pid auid=$(</proc/self/loginuid) \
        uid=$uid euid=$euid suid=$suid fsuid=$fsuid \
        gid=$gid egid=$egid sgid=$sgid fsgid=$fsgid \
	subj=$subj obj=$obj \
	a0=$a0
}

# if the object is not created, no object label is collected
function augrok_mls_name {
    augrok_default subj=$subj name=$name
}

function augrok_mls_search_fail {
    augrok_default subj=$subj
}

function augrok_mls_opid_label {
    declare -a aupids
    declare i a

    a=( {a..z} )
    for ((i=0; i<${#opid[*]}; i++)); do
	aupids+=( "opid#${a[i]}=${opid[i]}" "obj#${a[i]}=$obj" )
    done

    augrok_default --debug subj=$subj "${aupids[@]}"
}

######################################################################
# MAC test functions for context transformations
######################################################################

function compute_contexts {
    declare subj_default obj_default subj_def_label obj_def_label
    declare acc=$1

    # The default subject context is determined by a policy auto transition
    # rule.  The default object context is the current context we're running in.
    subj_default=$(test_context $(which do_$syscall)) || exit_error
    obj_default=$(cat /proc/self/attr/current)

    case $acc in
        eq)
            if [[ $syscall == mq_* ]]; then
		subj_def_label=SystemHigh
		obj_def_label=SystemHigh
	    else
		subj_def_label=SystemLow
		obj_def_label=SystemLow
            fi ;;
        dom)
	    subj_def_label=SystemHigh
	    obj_def_label=SystemLow ;;
        domby)
            subj_def_label=SystemLow
            obj_def_label=SystemHigh ;;
        incomp)
            subj_def_label="s1:c1.c3"
            obj_def_label="s1:c4.c6" ;;
        *) exit_error "test must specify an mls op [eq, dom, domby, incomp]" ;;
    esac

    subj=$(set_context_label $subj_default $subj_def_label)
    obj=$(set_context_label $obj_default $obj_def_label)
}

function set_context_label {
    declare context=$1 label=$2

    while [[ $context == *:*:*:* ]]; do
        context=${context%:*}
    done
    echo "$context:$label"
}

# It takes a bit of work to determine the correct fscreate context to set for
# new file objects. The final type is determined by a type transition rule,
# which uses a combination of the subject's type and the generic type for the
# filesystem where the new file will be created.
#
#
# The final context is a combination of the user and type transition source from
# the subject, the role and type transition target from the relevant filesystem,
# and the label that was specified on the testcase config line.
function set_fscreate_context {
    declare subject=$1 object=$2 fs=$3 fs_context
    declare t_source t_target n_user n_role n_type n_label

    fs_context=$(get_fsobj_context $fs)

    t_source=$(get_context_type $subject)
    t_target=$(get_context_type $fs_context)

    n_user=$(get_context_user $subject)
    n_role=$(get_context_role $fs_context)
    n_type=$(get_tmpfile_type $t_source $t_target)
    n_label=$(get_context_label $object)

    echo "$n_user:$n_role:$n_type:$n_label"
}

# set the label for filesystem objects
function set_fsobj_label {
    declare fsobj=$1 label=$2
    chcon --no-dereference -l $label $fsobj || exit_error
}

function get_context_label {
    echo "${1#*:*:*:}"
}

function get_context_role {
    declare type=${1#*:}
    echo "${type%%:*}"
}

function get_context_type {
    declare type=${1#*:*:}
    echo "${type%%:*}"
}

function get_context_user {
    echo "${1%%:*}"
}

# get the context for a filesystem object
function get_fsobj_context {
    echo $(ls --scontext -d $1 | awk '{print $1}')
}

# query system policy for the type that will be used for new tempfiles
function get_tmpfile_type {
    declare ftype

    ftype=$(sesearch --type --class file --source $1 --target $2 | awk '{print $6}')
    echo ${ftype%;}
}

######################################################################
# common functions for creating test objects
######################################################################

# create a tempdir <var> [basedir=dir] [context=context] [mode=mode]
function create_dir {
    declare tmpd var basedir context mode
    var=$1 ; shift
    eval "$(parse_named "$@")" || exit_error

    tmpd=$(mktemp -d ${basedir:+-p $basedir}) || exit_error
    prepend_cleanup "rm -rf $tmpd"
    [[ -n $mode ]] && { chmod 600 $tmpd ; chmod $mode $tmpd; }
    [[ -n $context ]] && set_fsobj_label $tmpd $(get_context_label $context)
    eval "$var=\$tmpd" || exit_error
}

# create a temporary executable <var> [basedir=dir] [context=context] [mode=mode]
function create_exec {
    declare tmpf var basedir context mode
    var=$1 ; shift
    eval "$(parse_named "$@")" || exit_error

    tmpf=$(mktemp ${basedir:+-p $basedir}) || exit_error
    prepend_cleanup "rm -f $tmpf"
    cp /bin/true $tmpf || exit_error
    chmod ${mode:-"u+x"} $tmpf
    [[ -n $context ]] && set_fsobj_label $tmpf $(get_context_label $context)
    eval "$var=\$tmpf" || exit_error
}

# create a tempfile <var> [basedir=dir] [context=context] [mode=mode]
function create_file {
    declare tmpf var basedir context mode
    var=$1 ; shift
    eval "$(parse_named "$@")" || exit_error

    tmpf=$(mktemp ${basedir:+-p $basedir}) || exit_error
    prepend_cleanup "rm -f $tmpf"
    [[ -n $mode ]] && { chmod 600 $tmpf ; chmod $mode $tmpf; }
    [[ -n $context ]] && set_fsobj_label $tmpf $(get_context_label $context)
    eval "$var=\$tmpf" || exit_error
}

# create a temporary symlink <var> [basedir=dir] [context=context] 
function create_symlink {
    declare tmpd symlink var basedir context
    var=$1 ; shift
    eval "$(parse_named "$@")" || exit_error

    tmpd=$(mktemp -d ${basedir:+-p $basedir}) || exit_error
    prepend_cleanup "rm -rf $tmpd"

    symlink="$tmpd/symlink"
    ln -s $tmp1 $symlink
    [[ -n $context ]] && set_fsobj_label $symlink $(get_context_label $context)
    eval "$var=\$symlink" || exit_error
}

# create a message queue and return the key <var> [context=context] 
function create_msg_key {
    declare ipc_key=50 var context
    var=$1; shift
    eval "$(parse_named "$@")" || exit_error

    prepend_cleanup "ipcrm -Q $ipc_key"
    ${context:+runcon $context} $(which do_msgget) $ipc_key create || exit_error
    eval "$var=\$ipc_key"
}

# create a message queue and return the queue id <var> [context=context] 
function create_msg_id {
    declare result id ipc_key=50 var context
    var=$1; shift
    eval "$(parse_named "$@")" || exit_error

    prepend_cleanup "ipcrm -Q $ipc_key"
    read result id foo <<<"$(${context:+runcon $context} $(which do_msgget) $ipc_key create)"
    [[ $result == 0 ]] || exit_error "could not create message queue"

    eval "$var=\$id"
}

# create a semaphore set <var> [context=context] 
function create_sem_key {
    declare ipc_key=50 var context
    var=$1; shift
    eval "$(parse_named "$@")" || exit_error

    prepend_cleanup "ipcrm -S $ipc_key"
    ${context:+runcon $context} $(which do_semget) $ipc_key create || exit_error
    eval "$var=\$ipc_key"
}

# create a semaphore set and return the set id <var> [context=context] 
function create_sem_id {
    declare result id ipc_key=50 var context
    var=$1; shift
    eval "$(parse_named "$@")" || exit_error

    prepend_cleanup "ipcrm -S $ipc_key"
    read result id foo <<<"$(${context:+runcon $context} $(which do_semget) $ipc_key create)"
    [[ $result == 0 ]] || exit_error "could not create semaphore set"

    eval "$var=\$id"
}

# create a shared memory segment <var> [context=context] 
function create_shm_key {
    declare ipc_key=50 var context
    var=$1; shift
    eval "$(parse_named "$@")" || exit_error

    ${context:+runcon $context} $(which do_shmget) $ipc_key create || exit_error
    prepend_cleanup "ipcrm -M $ipc_key"
    eval "$var=\$ipc_key"
}

# create a shared memory segment and return the segment id <var> [context=context] 
function create_shm_id {
    declare result id ipc_key=50 var context
    var=$1; shift
    eval "$(parse_named "$@")" || exit_error

    prepend_cleanup "ipcrm -M $ipc_key"
    read result id foo <<<"$(${context:+runcon $context} $(which do_shmget) $ipc_key create)"
    [[ $result == 0 ]] || exit_error "could not create shared memory segment"

    eval "$var=\$id"
}

# create a message queue <var> [context=context] 
function create_mq {
    declare mq_name="test_mq" var context
    var=$1; shift
    eval "$(parse_named "$@")" || exit_error

    ${context:+runcon $context} $(which do_mq_open) "/$mq_name" create \
        || exit_error

    prepend_cleanup "${context:+runcon $context} $(which do_mq_unlink) "/$mq_name""
    eval "$var=\$mq_name"
}

# register cleanup for a to-be-created message queue <var> [context=context] 
function create_mq_name {
    declare mq_name="test_mq" var context
    var=$1; shift
    eval "$(parse_named "$@")" || exit_error

    prepend_cleanup "${context:+runcon $context} $(which do_mq_unlink) "/$mq_name""
    eval "$var=\$mq_name"
}

# create a dummy process <var> [context=context] 
function create_process {
    declare mypid var context
    var=$1; shift
    eval "$(parse_named "$@")" || exit_error

    ${context:+runcon $context} $(which do_dummy) &
    mypid=$!

    prepend_cleanup "kill -SIGKILL $mypid"
    eval "$var=\$mypid"
}

# create a group of dummy processes <var> [context=context]
function create_pgrp {
    declare mypgid var context
    var=$1; shift
    eval "$(parse_named "$@")" || exit_error

    ${context:+runcon $context} $(which do_dummy_group) 20 &
    sleep 1 # let do_dummy_group get started
    mypgid=$(ps --no-headers -C do_dummy_group -o pgid | head -n1)

    prepend_cleanup "killall -SIGKILL do_dummy_group"
    eval "$var=\$mypgid"
}

######################################################################
# CAP functions for creating test objects or other test setup
######################################################################

function create_fs_objects_cap {
    declare p=$1 base all="a+rwx"

    case $p in
        dir_mount)
	    create_dir target mode="u+r"
	    source=none
	    flag=tmpfs
	    name=$target

	    prepend_cleanup "umount $target"
            ;;

	file_priv)
	    create_file target mode=$all
	    name=$target ;;

	file_swap)
	    create_file target mode="u+rwx"
	    name=$target

	    /bin/dd if=/dev/zero of=$target bs=1024 count=1024
	    /sbin/mkswap $target
	    prepend_cleanup "swapoff $target"

	    [[ $tag == *fail* ]]  && augrokfunc=augrok_default
	    ;;

	module_load)
	    target=/lib/modules/$(uname -r)/kernel/drivers/net/dummy.ko
	    augrokfunc=augrok_default
	    ;;

	module_unload)
	    target=dummy
	    augrokfunc=augrok_default
	    ;;

	secattr_*)
	    declare action=${perm##*_}

            create_file target mode="u+rw"
            name=$target
	    chmod a+r $target

	    flag=security.selinux
	    [[ $action == set ]] && value=$(cat /proc/self/attr/current)
	    ;;

	umask_set)
	    flag=022
	    augrokfunc=augrok_default
	    ;;

        *) exit_error "unknown perm to test: $p" ;;
    esac

    # special handling for fd syscalls, must do before *at handling
    if [[ $syscall == f* ]]; then
	inode=$(stat -c '%i' $target)
	augrokfunc=augrok_inode
    fi

    # special handling for *at syscalls
    if [[ -n $at ]]; then
	dirname=$target
	while [[ $dirname == /*/* ]]; do dirname=${dirname%/*}; done
	[[ -n $source ]] && source=${source#/*/}
	target=${target#/*/}
	name=${name#/*/}
    fi

    # augrok setup
    [[ -z $augrokfunc ]] && augrokfunc=augrok_name
}

function create_ipc_objects_cap {
    declare p=$1 type=${1%_*}
    declare msg_type=1

    [[ $(type -t create_$type) == function ]] || \
	exit_error "create_$type function does not exist"
    create_${type} target

    flag=${1##*_} # set operation flag

    # augrok setup
    [[ -n $op && -z $augrokfunc ]] && augrokfunc=augrok_op
}

function create_process_objects_cap {
    declare p=$1

    case $p in
        process_attach)
            create_process target context=$obj
            opid=$target ;;
        process_newns)
	    flag=newns ;;
    esac

    # set test operation flag
    flag=${p##*_}
}

function create_io_objects_cap {
    declare p=$1

    case $p in
	io_perm)
	    target=642	# port address
	    flag=1 ;;	# number of bytes
	io_priv)
	    flag=1 ;;	# process io privilege level
	port_priv)
	    target=24 	# site-dependent mail handling, unused
	    flag=0	# INADDR_ANY
	    [[ -n $op ]] && augrokfunc=augrok_op ;;
	fio_fibmap)
	    create_file target mode="a+rwx"
	    flag=FIBMAP ;;
	tty_setlock)
	    target="/dev/tty"
	    flag=TIOCSLCKTRMIOS ;;
	*) exit_error "unknown io perm to test: $p" ;;
    esac
}

function setup_time {
    declare p=$1

    case $p in
	time_set)
	    flag=$(do_getseconds) ;;
	time_zone_set)
	    flag=$(do_getseconds)
	    read zone dst <<<"$(do_gettimezone)" ;;
    esac

    augrokfunc=augrok_default
}

######################################################################
# DAC functions for creating test objects
######################################################################

function create_fs_objects_dac {
    declare p=$1 base all="a+rwx"

    case $p in

        # changes to files/dirs (tested object is always the target)
        dir_exec)
	    create_dir target mode="${dacugo:0:1}+rx"
	    name=$target ;;

        file_read)
            create_file target mode="${dacugo:0:1}+r"
            name=$target;;
        file_write)
            create_file target mode="${dacugo:0:1}+rw"
            name=$target;;
        file_exec)
            create_exec target mode="${dacugo:0:1}+rx"
            name=$target;;

        symlink_read)
            create_symlink target mode="${dacugo:0:1}+r"
            name=$target ;;

        xattr_*)
	    declare action=${perm##*_}

            create_file target mode="${dacugo:0:1}+rw"
	    chmod a+r $target
            name=$target

	    case $flag in
		*mime_type)
		    flag=user.mime_type
		    value="text/plain"
		    if [[ $action == remove ]]; then
			read result foo bar \
			    <<<"$(do_setxattr $target $flag "$value")"
			[[ $result == 0 ]] || exit_error "could not initialize xattr"
		    fi
		    ;;
	    esac
	    ;;

        # changes to directory entries
        dir_add_name)
            create_dir base mode="${dacugo:0:1}+rwx"
            target="$base/new"

            name="$base/" # audit adds a trailing /
	    [[ $tag == *fail* ]]  && augrokfunc=augrok_default

	    if [[ -n $which ]]; then
		create_dir base mode=$all
		create_file source basedir=$base mode=$all
	    fi ;;
        dir_remove_name)
            create_dir base mode="${dacugo:0:1}+rwx"
            case $entry in
		dir)  create_dir target basedir=$base mode=$all ;;
		file) create_file target basedir=$base mode=$all ;;
            esac

	    # see above comment regarding directory write tests
            name="$base/" # audit adds a trailing /
	    /bin/su - $TEST_USER -c "ls $base >/dev/null" || \
		augrokfunc=augrok_default

            # for syscalls that operate on more than one pathname
            # determine which is the actual test object
            case $which in
                old)
		    source=$target
		    create_dir base mode=$all
		    create_file target basedir=$base mode=$all ;;
                new)
		    create_dir base mode=$all
		    create_file source basedir=$base mode=$all ;;
            esac
            ;;

        *) exit_error "unknown perm to test: $p" ;;
    esac

    # special handling for fd syscalls, must do before *at handling
    if [[ $syscall == f* ]]; then
	inode=$(stat -c '%i' $target)
	augrokfunc=augrok_inode
    fi

    # special handling for *at syscalls
    if [[ -n $at ]]; then
	dirname=$target
	while [[ $dirname == /*/* ]]; do dirname=${dirname%/*}; done
	[[ -n $source ]] && source=${source#/*/}
	target=${target#/*/}
	name=${name#/*/}
    fi

    # augrok setup
    [[ -z $augrokfunc ]] && augrokfunc=augrok_name
}

function create_ipc_objects_dac {
    declare p=$1 type=${1%_*}
    declare msg_type=1

    [[ $(type -t create_$type) == function ]] || \
	exit_error "create_$type function does not exist"
    create_${type} target

    flag=${1##*_} # set operation flag

    # special setup for sending/recving messages
    case $p in
	*_send)
	    flag=$msg_type ;;
	*_recv)
	    declare result

	    flag=$msg_type
	    # always use do_msgsnd regardless of whether the syscall is ipc() or
	    # msgrcv() -- it's using a library call, so it works. we have a
	    # do_ipc utility purely because the harness wants to find a
	    # do_$syscall binary.
	    read result foo bar \
		<<<"$(do_msgsnd $target $flag 'test message')"
	    [[ $result == 0 ]] || exit_error "could not send initial message" ;;
    esac

    # augrok setup
    [[ -n $op && -z $augrokfunc ]] && augrokfunc=augrok_op
}

######################################################################
# MAC functions for creating test objects
######################################################################

# - may use these named parameters:
#   subj: the context in which the test operation will run 
#   obj: desired context for test object
#
# - may set these test operation parameters:
#   flag: test operation flag
#   dirname: for *at syscalls
#   source: for link,symlink,rename syscalls
#   target: test operation target object (required)
#
# - may set these augrok parameters:
#   name: file or mq name
#   opid: test object pid(s)
#   obj: may re-determine for audit record
function create_fs_objects_mac {
    declare p=$1 base
    
    # When creating filesystem objects, all objects other than the actual target
    # object (the one we're testing) should be created with $subj context to
    # avoid early failures on other permission checks.
    case $p in

        # changes to files/dirs (tested object is always the source)
        dir_reparent)
	    create_dir base context=$subj
	    create_dir source basedir=$base context=$obj
            target="$base/new"
            name=$source ;;
        file_link)
	    create_dir base context=$subj
	    create_file source basedir=$base context=$obj
            target="$base/new"
            name=$source ;;
        file_rename)
	    create_dir base context=$subj
	    create_file source basedir=$base context=$obj
            target="$base/new"
            name=$source ;;

        # changes to files/dirs (tested object is always the target)
        dir_mount)
	    create_dir target context=$obj
	    source=none
	    flag=tmpfs
	    name=$target

	    prepend_cleanup "umount $target" ;;
        dir_rmdir)
	    create_dir base context=$subj
	    [[ -n $which ]] && create_dir source basedir=$base context=$subj
	    create_dir target basedir=$base context=$obj
            name=$target ;;

        file_unlink)
	    create_dir base context=$subj
	    [[ -n $which ]] && create_file source basedir=$base context=$subj
	    create_file target basedir=$base context=$obj
            name=$target ;;
        file_read|file_write)
            create_file target context=$obj
            name=$target;;
        file_exec)
            create_exec target context=$obj
            name=$target;;
	file_create)
            create_dir target context=$subj

	    # determine the fscreate context, see comment for
	    # set_fscreate_context() for an explanation
	    setcontext=$(set_fscreate_context $subj $obj /tmp)

	    # setup other test utility args
            [[ -n $which ]] && source=$tmp1 # if new file is a symlink
            target="$target/new"

	    # setup augrok parameters
	    [[ $expres == fail ]] && augrokfunc=augrok_mls_name
	    name=$target
	    obj=$setcontext ;;
        symlink_read)
            create_symlink target context=$obj
            name=$target ;;

        # changes to directory entries
        dir_add_name)
            create_dir base context=$obj
            target="$base/new"

            name="$base/" # audit adds a trailing /
	    inode=$(runcon $subj -- stat -c '%i' $base)
	    if [[ -n $inode ]]; then
		# If the kernel fails to create the object, audit does not
		# update the value for the "name" field, so the inode number is
		# the reliable way to identify a directory.
		augrokfunc=augrok_mls_inode_label
	    else
		# These tests verify mls permission checks for directory writes.
		# In some scenarios, this means that we can't *read* the
		# directory either and the kernel will stop path resolution
		# before even attempting the operation. Note that this
		# implementation detail means we cannot truly verify a directory
		# write permission failure for some subj/obj combinations, but
		# we can prove that the operation won't succeed.
		augrokfunc=augrok_mls_search_fail
	    fi

	    if [[ -n $which ]]; then
		create_dir base context=$subj
		create_file source basedir=$base context=$subj
	    fi ;;
        dir_remove_name)
            create_dir base context=$obj
            case $entry in
		dir)  create_dir target basedir=$base context=$subj ;;
		file) create_file target basedir=$base context=$subj ;;
            esac

	    # see above comment regarding directory write tests
            name="$base/" # audit adds a trailing /
	    runcon $subj -- ls $base >/dev/null || \
		augrokfunc=augrok_mls_search_fail

            # for syscalls that operate on more than one pathname
            # determine which is the actual test object
            case $which in
                old)
                    source=$target
		    create_dir base context=$subj
		    create_file target basedir=$base context=$subj ;;
                new)
		    create_dir base context=$subj
		    create_file source basedir=$base context=$subj ;;
            esac
            ;;

        *) exit_error "unknown perm to test: $p" ;;
    esac

    # ensure filesystem object has correct type
    # must be done _before_ handling for *at calls
    # must not be done if the file doesn't exist yet
    [[ $p == file_create ]] || obj=$(get_fsobj_context $name)

    # special handling for *at syscalls
    if [[ -n $at ]]; then
	dirname=$target
	while [[ $dirname == /*/* ]]; do dirname=${dirname%/*}; done
	[[ -n $source ]] && source=${source#/*/}
	target=${target#/*/}
	name=${name#/*/}
    fi

    # augrok setup
    [[ -z $augrokfunc ]] && augrokfunc=augrok_mls_name_label
}

function create_ipc_objects_mac {
    declare p=$1 type=${1%_*}
    declare msg_type=1

    [[ $(type -t create_$type) == function ]] || \
	exit_error "create_$type function does not exist"
    create_${type} target context=$obj

    flag=${1##*_} # set operation flag

    # special setup for sending/recving messages
    case $p in
	*_send)
	    flag=$msg_type ;;
	*_recv)
	    declare result

	    flag=$msg_type
	    # always use do_msgsnd regardless of whether the syscall is ipc() or
	    # msgrcv() -- it's using a library call, so it works. we have a
	    # do_ipc utility purely because the harness wants to find a
	    # do_$syscall binary.
	    read result foo bar \
		<<<"$(runcon $obj -- do_msgsnd $target $flag 'test message')"
	    [[ $result == 0 ]] || exit_error "could not send initial message" ;;
    esac

    # augrok setup
    [[ -n $op && -z $augrokfunc ]] && augrokfunc=augrok_mls_op_label
    [[ -z $augrokfunc ]] && augrokfunc=augrok_mls_label
}

function create_mq_objects_mac {
    declare p=$1 mq_dir=/dev/mqueue

    if [[ $p == mq_create ]]; then
	create_mq_name target context=$obj

	mkdir $mq_dir ; mount -t mqueue mqueue $mq_dir
	    # determine the fscreate context, see comment for
	    # set_fscreate_context() for an explanation
	    setcontext=$(set_fscreate_context $subj $obj $mq_dir)
	umount $mq_dir ; rmdir $mq_dir

	[[ $expres == fail ]] && augrokfunc=augrok_mls_name
	obj=$setcontext
    else
	create_mq target context=$obj
	# get the real object type
	mkdir $mq_dir ; mount -t mqueue mqueue $mq_dir
	obj=$(get_fsobj_context "$mq_dir/$target")
	runcon $subj -- ls "$mq_dir/$target" || \
	    augrokfunc=augrok_mls_search_fail
	umount $mq_dir ; rmdir $mq_dir
    fi

    # set augrok params first, before we change $target
    [[ -z $augrokfunc ]] && augrokfunc=augrok_mls_name_label
    name=$target

    # set operation params
    target="/$target"
}

function create_process_objects_mac {
    declare p=$1

    case $p in
        process_*)
            create_process target context=$obj
            opid=$target ;;
        pgrp_*)
            create_pgrp target context=$obj
            opid+=( $(ps --no-headers -C do_dummy_group -o pid | sort -n) ) ;;
        *) exit_error "unknown perm to test: $p" ;;
    esac

    # set test operation flag
    flag=${p##*_}

    augrokfunc=augrok_mls_opid_label
}
