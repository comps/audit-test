#!/bin/bash
# =============================================================================
#   Copyright 2010, 2011 International Business Machines Corp.
#
#   This program is free software: you can redistribute it and/or modify
#   it under the terms of the GNU General Public License as published by
#   the Free Software Foundation, either version 2 of the License, or
#   (at your option) any later version.
#
#   This program is distributed in the hope that it will be useful,
#   but WITHOUT ANY WARRANTY; without even the implied warranty of
#   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#   GNU General Public License for more details.
#
#   You should have received a copy of the GNU General Public License
#   along with this program.  If not, see <http://www.gnu.org/licenses/>.
# =============================================================================

# This function adds a non-privleged user, becomes that user for the purpose
# of attempting to insert an ebtables rule. The operation is expected to fail
# and return a non-zero status, otherwise the test has failed because a
# non-privleged user should not be able to modify ebtables

source ../utils/functions.bash || exit 2

set -x

rc=0
export TEST_USER=testuser2
useradd -m -p usertest "$TEST_USER"
/bin/su - $TEST_USER -c "/sbin/ebtables -I INPUT 1 -i $LOCAL_SEC_DEV -j DROP"
rc=$?
if [[ $rc -ne 0 ]]; then
    echo "operation not permitted, return code is $rc"
    killall -9 -u "$TEST_USER"
    userdel -rf "$TEST_USER" &>/dev/null
    ebtables -L
    exit_pass
else
    echo "test failed, ebtables operation permitted"
    killall -9 -u "$TEST_USER"
    userdel -rf "$TEST_USER" &>/dev/null
    ebtables -L
    exit_fail
fi
exit
