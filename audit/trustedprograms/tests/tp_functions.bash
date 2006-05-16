#!/bin/bash
###############################################################################
# (c) Copyright Hewlett-Packard Development Company, L.P., 2005
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
PATH=$TOPDIR/utils:$PATH

source functions.bash

function generate_unique {
    declare file=$1
    declare i name id

    # find the first available name called testuser%d
    for ((i=1; i<100; i++)); do
        name=testuser$i
        grep -q "^$name:" "$file" || break
    done
    if [[ $i == 100 ]]; then
        echo "failed to find a unique name" >&2
        return 2
    fi

    # find the highest id under 65000, the nobody range
    id=$(sort -rnt: -k3 "$file" | awk -F: '$3<65000{print $3;exit}')
    if [[ ! -n $id ]]; then
        echo "error scanning $file" >&2
        return 2
    fi

    ((id++))
    if cut -d: -f3 "$file" | grep -Fxq $id; then
        echo "couldn't find a unique id ($id)" >&2
        return 2
    fi

    echo $name $id
}

function generate_unique_user {
    generate_unique /etc/passwd
}

function generate_unique_group {
    generate_unique /etc/group
}

function setpid {
    "$@" &
    pid=!$
    wait $pid
}

######################################################################
# common startup
######################################################################

read group gid <<<"$(generate_unique_group)"
read user uid <<<"$(generate_unique_user)"

append_cleanup "grep -q '^$user:' /etc/passwd && userdel '$group'"
append_cleanup "grep -q '^$group:' /etc/group && groupdel '$group'"

set -x
