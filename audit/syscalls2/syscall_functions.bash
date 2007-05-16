#!/bin/bash
###############################################################################
# (c) Copyright Hewlett-Packard Development Company, L.P., 2006
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
###############################################################################

export PATH=/usr/local/bin:/usr/bin:/bin:/usr/sbin:/sbin
if [[ -z $TOPDIR ]]; then
    TOPDIR=$(
    while [[ ! $PWD -ef / ]]; do
        [[ -f rules.mk ]] && { echo $PWD; exit 0; }
        cd ..
    done
    exit 1
    ) || { echo "Can't find TOPDIR, where is rules.mk?" >&2; exit 2; }
    export TOPDIR
fi
PATH=$TOPDIR/utils:$TOPDIR/utils/bin:$PATH

source testcase.bash

set -x

######################################################################
# global variables
######################################################################

# NOTE: these are not truly global since this file is sourced from inside
#       run_test(), so declare them with "declare"

# test utility args
declare op dirname source target flag setcontext

# audit record fields
declare log_mark syscall success pid auid exitval name
declare uid=0 euid=0 suid=0 fsuid=0
declare gid=0 egid=0 sgid=0 fsgid=0

# audit record fields - mac only
declare subj obj
declare -a opid

# for tests/test_init_module.c and tests/test_delete_module.c
export AUDIT_KMOD_DIR=/lib/modules/$(uname -r)/kernel/drivers/net
export AUDIT_KMOD_NAME=dummy

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

function setpid {
    "$@" &
    pid=$!
    wait $pid
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

function augrok_mls {
    augrok_default subj=$subj obj=$obj
}

function augrok_mls_name {
    augrok_default subj=$subj obj#a=$obj name#a=$name
}

function augrok_mls_inode {
    augrok_default subj=$subj obj#a=$obj inode#a=$inode
}

# if the object is not created, no object label is collected
function augrok_mls_create_fail {
    augrok_default subj=$subj name=$name
}

function augrok_mls_search_fail {
    augrok_default subj=$subj
}

function augrok_mls_opid {
    declare aupids

    # XXX REVISIT
    # grep is a workaround, should pass pid list to augrok
    # augrok collapses multiple obj's with same value
    aupids=$(augrok_default subj=$subj obj=$obj | \
        grep -o 'opid\(_[0-9]*\)\?=[0-9]*' | sed 's/.*=//' | sort -n | xargs)
        
    [[ "${opid[*]}" == "$aupids" ]] || return 1

    return 0
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
            if [[ $syscall == mq_* || $perm == *defcontext ]]; then
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
    append_cleanup "rm -rf $tmpd"
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
    append_cleanup "rm -f $tmpf"
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
    append_cleanup "rm -f $tmpf"
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
    append_cleanup "rm -rf $tmpd"

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

    append_cleanup "ipcrm -Q $ipc_key"
    ${context:+runcon $context} $(which do_msgget) $ipc_key create || exit_error
    eval "$var=\$ipc_key"
}

# create a message queue and return the queue id <var> [context=context] 
function create_msg_id {
    declare result id ipc_key=50 var context
    var=$1; shift
    eval "$(parse_named "$@")" || exit_error

    append_cleanup "ipcrm -Q $ipc_key"
    read result id foo <<<"$(${context:+runcon $context} $(which do_msgget) $ipc_key create)"
    [[ $result == 0 ]] || exit_error "could not create message queue"

    eval "$var=\$id"
}

# create a semaphore set <var> [context=context] 
function create_sem_key {
    declare ipc_key=50 var context
    var=$1; shift
    eval "$(parse_named "$@")" || exit_error

    append_cleanup "ipcrm -S $ipc_key"
    ${context:+runcon $context} $(which do_semget) $ipc_key create || exit_error
    eval "$var=\$ipc_key"
}

# create a semaphore set and return the set id <var> [context=context] 
function create_sem_id {
    declare result id ipc_key=50 var context
    var=$1; shift
    eval "$(parse_named "$@")" || exit_error

    append_cleanup "ipcrm -S $ipc_key"
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
    append_cleanup "ipcrm -M $ipc_key"
    eval "$var=\$ipc_key"
}

# create a shared memory segment and return the segment id <var> [context=context] 
function create_shm_id {
    declare result id ipc_key=50 var context
    var=$1; shift
    eval "$(parse_named "$@")" || exit_error

    append_cleanup "ipcrm -M $ipc_key"
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

    append_cleanup "${context:+runcon $context} $(which do_mq_unlink) "/$mq_name""
    eval "$var=\$mq_name"
}

# register cleanup for a to-be-created message queue <var> [context=context] 
function create_mq_name {
    declare mq_name="test_mq" var context
    var=$1; shift
    eval "$(parse_named "$@")" || exit_error

    append_cleanup "${context:+runcon $context} $(which do_mq_unlink) "/$mq_name""
    eval "$var=\$mq_name"
}

# create a dummy process <var> [context=context] 
function create_process {
    declare mypid var context
    var=$1; shift
    eval "$(parse_named "$@")" || exit_error

    ${context:+runcon $context} $(which do_dummy) &
    mypid=$!

    append_cleanup "kill -SIGKILL $mypid"
    eval "$var=\$mypid"
}

# create a group of dummy processes <var> [context=context]
function create_pgrp {
    declare mypgid var context
    var=$1; shift
    eval "$(parse_named "$@")" || exit_error

    ${context:+runcon $context} $(which do_dummy_group) 30 &
    sleep 1 # let do_dummy_group get started
    mypgid=$(ps --no-headers -C do_dummy_group -o pgid | head -n1)

    append_cleanup "killall -SIGKILL do_dummy_group"
    eval "$var=\$mypgid"
}
