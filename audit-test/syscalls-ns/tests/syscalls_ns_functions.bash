###############################################################################
#   Copyright (c) 2014 Red Hat, Inc. All rights reserved.
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
source namespace_functions.bash || exit 2

#
# simplified functions for generic namespacing
# - abstract ns set parameter, use global exported var
#

create_ns()
{
    declare -g -x NS_SET
    NS_SET=$(create_nsset "$@") || exit_error "create_nsset failed"
    prepend_cleanup "kill -9 \"$NS_SET\""
    return 0
}
exec_ns()
{
    exec_nsset "$NS_SET" "$@"
    return $?
}

#
# environment preparation functions
# (NOTE: these functions need already created namespace set! (NS_SET))
#

# prepare environment for mnt namespace testing
#
# As an added feature, more files aside from the regular ones are created,
# specifically ${prefix}d (empty dir), ${prefix}x (valid executable) and
# ${prefix}s (symlink) where ${prefix} is one of MNT_INFO paths.
# These special files have the same access and namespace properties as
# the regular MNT_INFO paths, they exist purely to help with testing.
#
# exports global array MNT_INFO, which contains
# - [0]: file path accessible only in the original NS (A/a/a)
# - [1]: file path accessible only in the new NS (A/b/b)
# - [2]: file path accessible from both, but read/writable only
#        in the original NS (A/c/c)
#
create_env_mnt()
{
    local tmp_a= tmp_b=
    local i=

    # /tmp is noexec, use / as TMPDIR
    tmp_a=$(mktemp -p / -d)
    tmp_b=$(mktemp -p / -d)
    prepend_cleanup "rm -rf \"$tmp_a\" \"$tmp_b\""

    # base directories
    mkdir "$tmp_a"/{a,c} "$tmp_b"/{b,c} || exit_error
    # - regular files
    touch "$tmp_a"/{a/a,c/c} "$tmp_b"/{b/b,c/c} || exit_error
    # - empty dirs
    mkdir "$tmp_a"/{a/ad,c/cd} "$tmp_b"/{b/bd,c/cd} || exit_error
    # - executables
    for i in "$tmp_a"/{a/ax,c/cx} "$tmp_b"/{b/bx,c/cx}; do
        cp /bin/true "$i" || exit_error
    done;
    # - symlinks
    for i in "$tmp_a"/{a/as,c/cs} "$tmp_b"/{b/bs,c/cs}; do
        ln -s / "$i" || exit_error
    done;

    # adjust owner/mode on the "c" base dir and all file types inside,
    # except the symlink
    chmod 0000 "$tmp_b"/c/{c,cd,cx} || exit_error
    chown bin "$tmp_b"/c/{c,cd,cx} || exit_error
    chmod 0000 "$tmp_b"/c || exit_error
    chown bin "$tmp_b"/c || exit_error

    # to make this work correctly with shared /, the target mountpoint needs
    # to be on a privately-mounted filesystem - we could do `--make-rprivate /'
    # in the new ns, but that would be destructive if the ns logic fails
    # - instead, we "create" an already existing mountpoint out of the target
    #   directory, make it private and *then* bindmount other stuff on it
    prepend_cleanup "exec_ns umount \"$tmp_a\"; exec_ns umount \"$tmp_a\";"
    exec_ns mount --bind "$tmp_a" "$tmp_a" || exit_error
    exec_ns mount --make-private "$tmp_a" || exit_error
    exec_ns mount --bind "$tmp_b" "$tmp_a" || exit_error

    declare -g -a MNT_INFO=()
    MNT_INFO+=("$tmp_a"/a/a)
    MNT_INFO+=("$tmp_a"/b/b)
    MNT_INFO+=("$tmp_a"/c/c)

    return 0
}

# prepare environment for uts namespace testing
#
# exports global array UTS_INFO, which contains
# - [0]: original system hostname
# - [1]: original system domainname
#
create_env_uts()
{
    local host= domain=

    # not much, at least backup the original host/domain name
    # and export it
    host=$(hostname) || exit_error
    domain=$(domainname) || exit_error
    prepend_cleanup "hostname \"$host\""
    prepend_cleanup "domainname \"$domain\""

    declare -g -a UTS_INFO=()
    UTS_INFO+=("$host")
    UTS_INFO+=("$domain")

    return 0
}

# prepare environment for ipc namespace testing
#
# exports global array IPC_INFO, which contains
# - [0]: ipc key read/writable (6xx) only in the original NS (all ipc types)
# - [1]: ipc key read/writable (6xx) only in the new NS (all ipc types)
# - [2]: ipc key existing in both, but read/writable only in the original NS
#
create_env_ipc()
{
    local key_a=12345 key_b=12346 key_c=12347

    # original namespace
    prepend_cleanup "ipcrm -M \"$key_a\" -Q \"$key_a\" -S \"$key_a\""
    ipcrm -M "$key_a" -Q "$key_a" -S "$key_a"
    do_msgget "$key_a" create || exit_error
    do_semget "$key_a" create || exit_error
    do_shmget "$key_a" create || exit_error

    # new namespace
    # - the cleanup shouldn't be needed, but in case namespacing fails ..
    prepend_cleanup "exec_ns ipcrm -M \"$key_b\" -Q \"$key_b\" -S \"$key_b\""
    exec_ns ipcrm -M "$key_b" -Q "$key_b" -S "$key_b"
    exec_ns do_msgget "$key_b" create || exit_error
    exec_ns do_semget "$key_b" create || exit_error
    exec_ns do_shmget "$key_b" create || exit_error

    # additional objects in the new namespace, with keys identical to
    # the original namespace, but with owner/mode set to bin/000
    # - if the namespacing logic fails, these will replace the original
    #   objects in the original namespace, making the tests fail
    prepend_cleanup "ipcrm -M \"$key_c\" -Q \"$key_c\" -S \"$key_c\""
    prepend_cleanup "exec_ns ipcrm -M \"$key_c\" -Q \"$key_c\" -S \"$key_c\""
    ipcrm -M "$key_c" -Q "$key_c" -S "$key_c"
    exec_ns ipcrm -M "$key_c" -Q "$key_c" -S "$key_c"
    do_msgget "$key_c" create || exit_error
    do_semget "$key_c" create || exit_error
    do_shmget "$key_c" create || exit_error
    exec_ns run-as.py -u bin do_msgget "$key_c" create:000 || exit_error
    exec_ns run-as.py -u bin do_semget "$key_c" create:000 || exit_error
    exec_ns run-as.py -u bin do_shmget "$key_c" create:000 || exit_error

    declare -g -a IPC_INFO=()
    IPC_INFO+=("$key_a")
    IPC_INFO+=("$key_b")
    IPC_INFO+=("$key_c")

    return 0
}

# prepare environment for POSIX mq namespace testing
#
# exports global array MQ_INFO, which contains
# - [0]: mqueue name accessible only in the original NS
# - [1]: mqueue name accessible only in the new NS
# - [2]: mqueue name accessible from both, but read/writable only
#        in the original NS
#
create_env_mq()
{
    local mq_a="$(mktemp -u -p /)"
    local mq_b="$(mktemp -u -p /)"
    local mq_c="$(mktemp -u -p /)"

    # original namespace, new namespace and additional (shared) objects
    # for POSIX message queues
    prepend_cleanup "do_mq_unlink \"$mq_a\""
    do_mq_unlink "$mq_a"
    do_mq_open "$mq_a" create

    # new namespace
    prepend_cleanup "exec_ns do_mq_unlink \"$mq_b\""
    exec_ns do_mq_unlink "$mq_b"
    exec_ns do_mq_open "$mq_b" create

    # additional objects with shared name
    prepend_cleanup "do_mq_unlink \"$mq_c\"; exec_ns do_mq_unlink \"$mq_c\";"
    do_mq_unlink "$mq_c"
    do_mq_open "$mq_c" create
    exec_ns do_mq_unlink "$mq_c"
    exec_ns run-as.py -u bin do_mq_open "$mq_c" create:0000

    declare -g -a MQ_INFO=()
    MQ_INFO+=("$mq_a")
    MQ_INFO+=("$mq_b")
    MQ_INFO+=("$mq_c")

    return 0
}

# prepare environment for pid namespace testing
# - use a "placeholder P" (testing) process in the original namespace
#   that the new namespace can test against (ie. kill(2))
# - use a "placeholder C" (testing) process in the new namespace, to verify
#   that a process in the original namespace can affect the new namespace
#   using PIDs from the original namespace
# - this process needs to have pid >= 100 -- if the namespacing logic succeeds,
#   new PIDs will start from 1 and no reasonable test should exceed 99 process
#   spawns with its testing logic
#   - please note that pid re-usage occurs from 300 to kernel.pid_max on Linux
#     (Mac / HP-UX use 100 as lower limit) - assuming that the system uses pids
#     >=300 after boot, this loop should never need to run more than once
#     (but any sanity check is a good check)
#
# exports global array PID_INFO, which contains
# - [0]: PID of the "placeholder P" process in the original namespace
# - [1]: PID of the "placeholder C" process in the original namespace
# - [2]: PID of the "placeholder C" process in the new namespace
#
create_env_pid()
{
    local pid=

    declare -g -a PID_INFO=()

    # what to use as placeholders
    local holder_p_bin="${PLACEHOLDER_P_BIN:-sleep 3600}"
    local holder_c_bin="${PLACEHOLDER_C_BIN:-sleep 3600}"

    # "placeholder P"
    while [ -z "$pid" ] || [ "$pid" -le 100 ]; do
        if [ "$pid" ]; then
            kill -9 "$pid"
            wait "$pid" 2>/dev/null
        fi
        eval "$holder_p_bin &"
        pid="$!"
    done
    prepend_cleanup "kill -9 \"$pid\"; wait \"$pid\" 2>/dev/null;"
    PID_INFO+=("$pid")

    # "placeholder C"
    eval "exec_ns initwrap "$holder_c_bin" &"
    pid="$!"
    # the `&' spawns the exec_ns function itself in a subshell,
    # which then calls setns on initwrap, which forks the placeholder,
    # therefore the placeholder is child of initwrap, which itself is
    # child of the subshell
    #
    # wait for bash (subshell) to fork setns, which calls execve initwrap
    # (therefore wait for the subshell to have an "initwrap" child)
    pid=$(wait_for_child_pname "$pid" "initwrap") || exit_error
    # wait for the initwrap to fork and execve a "placeholder" child
    holder_c_bin=$(eval echo "$holder_c_bin") # simplify for ps
    pid=$(wait_for_child_pname "$pid" "$holder_c_bin" full) || exit_error
    # "pid" now contains outside (namespace-wise) pid of the placeholder

    prepend_cleanup "kill -9 \"$pid\"; wait \"$pid\" 2>/dev/null;"
    PID_INFO+=("$pid")

    # "placeholder C", new namespace
    # - when using simple sleep as a placeholder, we have no easy way of
    #   figuring out its pid in the new namespace (without doing something
    #   unreliable like 'exec_ns pidof sleep'), however as long as the test
    #   is sane and calls this function only once and right after create_ns,
    #   we can deterministically state that the placeholder will be the first
    #   spawned process after init and therefore have pid 2
    PID_INFO+=("2")

    return 0
}

set -x

# vim: sts=4 sw=4 et :
