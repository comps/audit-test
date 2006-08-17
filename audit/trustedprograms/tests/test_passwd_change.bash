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
# Verify audit of user password change.

source tp_functions.bash || exit 2

# setup
useradd -n -u $uid $user || exit_error "useradd failed"

# test
newpass=$(date +%s)
expect -c "
    spawn passwd $user
    expect {
        -nocase \"new unix password:\" {send \"$newpass\\r\"; exp_continue}
        eof
    }
    set pidfile [open \"$tmp1\" w]
    puts \$pidfile [exp_pid]"
pid=$(<$tmp1)

for msg_1 in \
    "PAM: chauthtok acct=$user : exe=./usr/bin/passwd.*res=success.*"
do
    augrok -q type=USER_CHAUTHTOK \
            user_pid=$pid \
            uid=$EUID \
            auid=$(</proc/self/loginuid) \
            msg_1=~"$msg_1" || exit_fail "missing: \"$msg_1\""
done

exit_pass
