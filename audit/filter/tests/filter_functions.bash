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

function get_event_obj {
    declare op=$1
    declare fs_obj
    
    case $op in
        file) fs_obj=$tmp1 ;;
        link)
            ln $tmp1 $tmp1.$op
            fs_obj=$tmp1.$op
            ;;
        symlink)
            ln -s $tmp1 $tmp1.$op
            fs_obj=$tmp1.$op
            ;;
        *) exit_error "unknown fs object type" ;;
    esac
    echo $fs_obj

    return 0
}

function get_fs_dev {
    df "$1" | grep -v ^Filesystem | head -n1 | awk '{print $1}'
}

function do_open_file {
    [[ -e $1 ]] || exit_error "$1 does not exist"

    if [[ $2 == "fail" ]]; then
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
