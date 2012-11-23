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
# Verify audit of user removal, including removal of mail file and home
# directory.

source tp_auth_functions.bash || exit 2

# setup
useradd -n -m -u $uid $user || exit_error "useradd failed"

# test
setpid userdel -r $user || exit_error "userdel failed"

msg_type=DEL_USER
grep "release 5" /etc/redhat-release &&  msg_type=USER_CHAUTHTOK

for msg_1 in \
    "op=deleting user entries id=$uid exe=\"*\.*/usr/sbin/userdel\"*.*res=success.*" \
    "op=deleting mail file id=$uid exe=\"*\.*/usr/sbin/userdel\"*.*res=success.*" \
    "op=deleting home directory id=$uid exe=\"*\.*/usr/sbin/userdel\"*.*res=success.*"
do
    augrok -q type=$msg_type \
            user_pid=$pid \
            uid=$EUID \
            auid=$(</proc/self/loginuid) \
            msg_1=~"$msg_1" || exit_fail "missing: \"$msg_1\""
done

exit_pass
