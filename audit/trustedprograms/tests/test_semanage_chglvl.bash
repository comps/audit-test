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
# Use semanage to change the level range of a user
# Start with the sample test user, and change their level range from the
# default to Unclassified-Secret then verify that the change was audited

source tp_auth_functions.bash || exit 2

# setup
range=s1-s2
seuser=staff_u
auid=$(cat /proc/self/loginuid)
useradd -Z staff_u -n -u $uid $user || exit_error "useradd failed"

# test
semanage login -m -r $range $user

# verify
if [ $? -ne 0 ]; then
  exit_error "semange returned an error"
fi

msg_1="op=modify selinux user mapping acct=\"*$user\"* old-seuser=$seuser old-role=\? old-range=s0 new-seuser=$seuser new-role=\? new-range=$range exe=/usr/sbin/semanage.*res=success.*"

augrok -q type=USER_ROLE_CHANGE auid=$auid \
  msg_1=~"$msg_1" || exit_fail "missing: \"$msg_1\""

# cleanup 
# deluser handled by tp_auth_functions

exit_pass
