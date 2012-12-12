#!/bin/bash
###############################################################################
# (c) Copyright Hewlett-Packard Development Company, L.P., 2005
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
# Verify audit of attempts to modify group password as an ordinary user.

source tp_auth_functions.bash || exit 2

# setup
groupadd -g $gid $group || exit_error "groupadd failed"

# test
newpass=$(date +%s)
chown $TEST_USER "$tmp1"
su $TEST_USER -c "
    expect -c '
        spawn gpasswd $group
        expect {
            -nocase \"new password:\" {send \"$newpass\"; exp_continue}
            eof
        }
        set pidfile [open \"$tmp1\" w]
        puts \$pidfile [exp_pid]'"
pid=$(<$tmp1)

op="testuser failed to change password of group testuser1: Permission denied"
msg_1="op=$op acct=\"$group\" exe=\"/usr/bin/gpasswd.*res=failed"

augrok -q type=USER_CHAUTHTOK \
    pid=$pid \
    uid=$(id -u $TEST_USER) \
    auid=$(</proc/self/loginuid) \
    msg_1=~"$msg_1" || exit_fail "missing: \"$msg_1\""

exit_pass
