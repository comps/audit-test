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
# Verify audit of group password change.

source tp_auth_functions.bash || exit 2

# setup
groupadd -g $gid $group || exit_error "groupadd failed"

# test
newpass=$(date +%s)
expect -c "
    spawn gpasswd $group
    expect {
        -nocase \"new password:\" {send \"$newpass\\n\"; exp_continue}
        eof
    }
    set pidfile [open \"$tmp1\" w]
    puts \$pidfile [exp_pid]"
pid=$(<$tmp1)

op="changing password"
grep "release 6" /etc/redhat-release && \
    op="password of group testuser1 changed by root"
msg_1="op=$op acct=\"*$group\"* exe=./usr/bin/gpasswd.*res=success.*"

augrok -q type=USER_CHAUTHTOK \
    user_pid=$pid \
    uid=$EUID \
    auid=$(</proc/self/loginuid) \
    msg_1=~"$msg_1" || exit_fail "missing: \"$msg_1\""

exit_pass
