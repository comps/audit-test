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
# Verify audit of changes to a user's default group.

source tp_auth_functions.bash || exit 2

# setup
useradd -n -u $uid $user || exit_error "useradd failed"
read group2 gid2 <<<"$(generate_unique_group)"
groupadd -g $gid2 $group2 || exit_error "groupadd failed"

prepend_cleanup "grep -q '^$group2:' /etc/group && groupdel '$group2'"

# test
setpid usermod -g $gid2 $user || exit_error "usermod failed"

for msg_1 in \
    "op=changing primary group acct=$user exe=./usr/sbin/usermod.*res=success.*"
do
    augrok -q type=USER_CHAUTHTOK \
            user_pid=$pid \
            uid=$EUID \
            auid=$(</proc/self/loginuid) \
            msg_1=~"$msg_1" || exit_fail "missing: \"$msg_1\""
done

exit_pass
