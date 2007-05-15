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
# 
# PURPOSE:
# Verify audit of an unsuccessful user password change.

source tp_auth_functions.bash || exit 2

# test
newpass=123
chown $TEST_USER "$tmp1"
su $TEST_USER -c "
    expect -c '
        spawn passwd
        expect {
            -nocase \"password:\" {send \"$newpass\\r\"; exp_continue}
            eof
        }
        set pidfile [open \"$tmp1\" w]
        puts \$pidfile [exp_pid]'"
pid=$(<$tmp1)

for msg_1 in \
    "PAM: chauthtok acct=$TEST_USER : exe=./usr/bin/passwd.*res=failed.*"
do
    augrok -q type=USER_CHAUTHTOK \
            user_pid=$pid \
            uid=$(id -u $TEST_USER) \
            auid=$(</proc/self/loginuid) \
            msg_1=~"$msg_1" || exit_fail "missing: \"$msg_1\""
done

exit_pass
