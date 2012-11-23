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
# Verify that newrole -r fails when the selinux user cannot access that role
# This test should be run from lspp_test_r which is only accessible to staff_u
# attempt to change to the user_r role which is only accessible from user_u

source testcase.bash || exit 2

# setup
role_target=user_r

semanage user -l | grep $(id -Z | awk -F: '{print $1}') | grep $role_target
if [ $? -ne 1 ]; then
  exit_fail "$role_target is accessible from the current role"
fi

# test
expect -c "
  spawn  newrole -r $role_target
  expect {
    Authenticating {exit -1}
    -re \".*is not a valid context$\" {exit 0}
  }"

# verify
if [ $? -eq 255 ]; then
  exit_fail "newrole accepted changing to a role that should have been inaccessible"
fi

exit_pass
