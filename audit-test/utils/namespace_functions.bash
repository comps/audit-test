###############################################################################
#   Copyright (c) 2015 Red Hat, Inc. All rights reserved.
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

PATH="$TOPDIR/utils/namespaces:$PATH"

#
# functions useful for linux namespace testing
#
# these work on namespace "sets" using a dummy processes to "hold" the sets
# - a set is then defined as all namespaces of a given PID
#
# example:
#   $ local pid=
#   $ pid=$(create_ns mnt ipc net) || error "creating nsset failed"
#   $ exec_ns $pid cat /some/file

# unshare a set of namespaces, storing it in a dummy process (returning its pid)
create_nsset()
{
    local unshare_args= ns= pid=
    declare -A nspaces

    for ns in "$@"; do
        case "$ns" in
            mnt|uts|ipc|net|pid|user)  nspaces["$ns"]=1 ;;
            mount)    nspaces['mnt']=1 ;;
            network)  nspaces['net']=1 ;;
            usr)      nspaces['user']=1 ;;
            *) return 1 ;;
        esac
    done

    for ns in "${!nspaces[@]}"; do
        case "$ns" in
            mnt)  unshare_args+="--mount " ;;
            *)    unshare_args+="--${ns} " ;;
        esac
    done

    if [ "${nspaces['pid']}" ]; then
        # special case - namespace is unshared only *after* additional fork
        # (or clone) and pids 0 and 1 need to be running for the ns to exist
        # - the ns itself exists since the second forked process, so we need
        #   to get pid of it, instead of our child
        unshare $unshare_args initwrap pause >/dev/null &
        # wait for unshare to exec initwrap / to fork pause
        pid=$(wait_for_child_pname "$!" pause) || \
            { [ "$!" ] && kill "$!"; return 1; }
    else
        # just spawn a dummy proces in the new namespaces
        # - redirect stdout to stop subshell running this func from waiting
        unshare $unshare_args pause >/dev/null &
        # wait for unshare to exec pause
        wait_for_pname "$!" pause || \
            { [ "$!" ] && kill "$!"; return 1; }
        pid="$!"
    fi

    echo "$pid"
    return 0
}

# execute given command in a namespace set defined a pid ($1)
exec_nsset()
{
    local pid="$1"
    shift

    setns -p "$pid" "$@"

    return $?
}

# vim: sts=4 sw=4 et :
