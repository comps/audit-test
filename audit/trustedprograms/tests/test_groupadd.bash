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
# Verify audit of new group creation.

source tp_auth_functions.bash || exit 2

# test
setpid groupadd -g $gid $group || exit_error "groupadd failed"

if ! augrok -q type=USER_CHAUTHTOK \
        user_pid=$pid \
        uid=$EUID \
        auid=$(</proc/self/loginuid) \
        msg_1=~"op=adding group acct=$group exe=./usr/sbin/groupadd.*res=success.*"; then
    exit_fail "failed to find audit.log entry"
fi

exit_pass
