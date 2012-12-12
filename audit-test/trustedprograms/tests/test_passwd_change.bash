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
# 
# PURPOSE:
# Verify audit of user password change.

source tp_auth_functions.bash || exit 2

# setup
useradd -N -u $uid $user || exit_error "useradd failed"

# test
newpass=$(date +OsLO\!%sMo)
expect -c "
    spawn passwd $user
    expect {
        -nocase -re \"New\.\*password:\" {send \"$newpass\\r\"; exp_continue}
        -nocase -re \"Retype new\.\*password:\" {send \"$newpass\\r\"; exp_continue}
        eof
    }
    set pidfile [open \"$tmp1\" w]
    puts \$pidfile [exp_pid]"
pid=$(<$tmp1)

for msg in "op=PAM:chauthtok acct=\"$user\" exe=\"/usr/bin/passwd\".*res=success" \
    "op=change password id=$uid exe=\"/usr/bin/passwd\".*res=success" ; do
    augrok -q type=USER_CHAUTHTOK \
        pid=$pid \
        uid=$EUID \
        auid=$(</proc/self/loginuid) \
        msg_1=~"$msg" || exit_fail "missing: \"$msg\""
done

exit_pass
