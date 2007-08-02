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
# Verify audit of changes to a user's login name.

source tp_auth_functions.bash || exit 2

# setup
useradd -n -G games -u $uid $user || exit_error "groupadd failed"
read user2 uid2 <<<"$(generate_unique_user)"
prepend_cleanup "userdel -r $user2"

# test
setpid usermod -l $user2 $user || exit_error "usermod failed"

# these messages are very inconsistent, sometimes reporting the new user,
# sometimes reporting the old.
for msg_1 in \
    "op=changing name acct=$user2 exe=./usr/sbin/usermod.*res=success.*" \
    "op=changing group member acct=$user2 exe=./usr/sbin/usermod.*res=success.*" \
    "op=changing member in shadow group acct=$user exe=./usr/sbin/usermod.*res=success.*" \
    "op=changing mail file name acct=$user2 exe=./usr/sbin/usermod.*res=success.*"
do
    augrok -q type=USER_CHAUTHTOK \
            user_pid=$pid \
            uid=$EUID \
            auid=$(</proc/self/loginuid) \
            msg_1=~"$msg_1" || exit_fail "missing: \"$msg_1\""
done

exit_pass
