#!/bin/bash
###############################################################################
# (c) Copyright Hewlett-Packard Development Company, L.P., 2007
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
# Successfully newrole to another role and verify the audit
# The test starts at lspp_test_r which has access to staff_r
# verify that using newrole to change roles works, and that it is audited

source testcase.bash || exit 2

# setup
old_context=`id -Z`
exp_context=staff_u:staff_r:staff_t:SystemLow-SystemHigh
auid=$(cat /proc/self/loginuid)

# test
expect -c "
  spawn  newrole -r staff_r
  expect {
    -nocase Authenticating {exp_continue}
    -nocase \"Password: \" {send \"$PASSWD\\r\"; exp_continue}
    -re \".*is not a valid context$\" {exit -1}
    -re \"bash: .*Permission denied$\" {exp_continue}
    -re \"\[.*staff_r.*\]# $\" {exit 0}
  }
"

# verify
if [ $? -ne 0 ]; then
  exit_fail "newrole failed to change roles"
fi

msg_1="newrole: old-context=${old_context} new-context=${exp_context}.*"
augrok -q type=USER_ROLE_CHANGE auid=$auid \
  msg_1=~"$msg_1" || exit_fail "missing: \"$msg_1\""

exit_pass
