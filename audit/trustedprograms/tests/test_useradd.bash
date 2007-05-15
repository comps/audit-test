#!/bin/bash
###############################################################################
# (c) Copyright Hewlett-Packard Development Company, L.P., 2005
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
# Verify audit of new user creation, including adding the new user to a group
# and creating the new user's home directory.

source tp_auth_functions.bash || exit 2

# test
setpid useradd -n -m -G games -u $uid -d /home/$user $user \
    || exit_error "useradd failed"

for msg_1 in \
    "op=adding user acct=$user exe=./usr/sbin/useradd.*res=success.*" \
    "op=adding user to group acct=$user exe=./usr/sbin/useradd.*res=success.*" \
    "op=adding user to shadow group acct=$user exe=./usr/sbin/useradd.*res=success.*" \
    "op=adding home directory acct=$user exe=./usr/sbin/useradd.*res=success.*"
do
    augrok -q type=USER_CHAUTHTOK \
            user_pid=$pid \
            uid=$EUID \
            auid=$(</proc/self/loginuid) \
            msg_1=~"$msg_1" || exit_fail "missing: \"$msg_1\""
done

exit_pass
