#!/bin/bash
###############################################################################
# (c) Copyright Hewlett-Packard Development Company, L.P., 2006, 2007
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
# 
# PURPOSE:
# Verify audit of setting selinux booleans.

source testcase.bash || exit 2

unset orig control
unset bool value
bool=$1
value=$2

function resolve_bool {
    declare v=$1

    case $v in
        on|true|1)
            echo 1 ;;
        off|false|0)
            echo 0 ;;
        *)  exit_error "getsebool: unknown output $v" ;;
    esac
}

function setup_bool {

    orig=$(getsebool $bool | awk '{print $3}')
    orig=$(resolve_bool $orig)
    prepend_cleanup "setsebool $bool $orig"

    # the starting value for the boolean must be the opposite of what
    # we're testing, otherwise there will be no audit record
    control=$(resolve_bool $value)
    control=$(( ! control ))
    [[ $control != $orig ]] && setsebool $bool $control
}

function check_bool_result {
    declare result=$1

    case $(resolve_bool $value) in
        0) [[ $result == 0 ]] && exit_error "unexpected test result" ;;
        1) [[ $result != 0 ]] && exit_error "unexpected test result" ;;
    esac
}

# setup
set -x
setup_bool

# test boolean
setsebool $bool $value
case $bool in
    user_ping)
        # we use rolecall with the staff_t domain instead of runcon
        # with a test domain to avoid having to add policy which would
        # allow the ping_t domain to write to the terminal
        rolecall -t staff_t -- ping -c 2 localhost ;;
    *)  exit_error "no test implemented for $bool" ;;
esac

# verify test result
check_bool_result $?

# verify audit record
value=$(resolve_bool $value)
augrok type==MAC_CONFIG_CHANGE \
        bool=$bool val=$value old_val=$control \
        auid=$(</proc/self/loginuid) \
            || exit_fail "missing audit record"

exit_pass
