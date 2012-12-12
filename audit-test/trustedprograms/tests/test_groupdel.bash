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
# Verify audit of group removal.

source tp_auth_functions.bash || exit 2

# setup
groupadd -g $gid $group || exit_error "groupadd failed"

# test
setpid groupdel $group || exit_error "groupdel failed"

for msg in "op=removing group from /etc/group id=$gid exe=\"/usr/sbin/groupdel\".*res=success" \
    "op=removing group from /etc/gshadow id=$gid exe=\"/usr/sbin/groupdel\".*res=success" \
    "op= id=$gid exe=\"/usr/sbin/groupdel\".*res=success" ; do

    augrok -q type=DEL_GROUP \
        pid=$pid \
        uid=$EUID \
        auid=$(</proc/self/loginuid) \
        msg_1=~"$msg" || exit_fail "missing: \"$msg\""
done

exit_pass
