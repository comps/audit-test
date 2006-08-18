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
PATH=$TOPDIR/utils:$PATH

source functions.bash

function get_fs_dev {
    declare fs_dev
    declare fs_mount=$(dirname $1)

    while [ "$(mount | awk -v DIR=$fs_mount '{ if ( $3 == DIR ) print $3 }')" = "" ]; do
        fs_mount=$(dirname $fs_mount)
    done

    fs_dev="$(mount | awk -v DIR=$fs_mount '{ if ( $3 == DIR ) print $1 }')"
    echo $fs_dev

    return 0
}

function do_open_file {
    [[ -e "$1" ]] || exit_error "$1 does not exist"

    if [[ -n $2 && $2 = "fail" ]]; then
        chmod 0600 $1 \
            || exit_error "unable to set the permissions on the test file"
        chown root:root $1 \
            || exit_error "unable to set the permissions on the test file"

        [[ -n $TEST_USER ]] \
            || exit_error "run in the harness or define \$TEST_USER"
        /bin/su $TEST_USER bash -c "cat \"$1\"" 2> /dev/null

        return 0
    fi

    cat "$1" > /dev/null
}

set -x
