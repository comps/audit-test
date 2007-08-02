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
# Use semanage to attempt to change to an invalid SELinux user for a user
# Start with the sample test user, and set their initial SELinux login user
# then verify that attempting to change to an invalid SELinux user will fail

source tp_auth_functions.bash || exit 2

# setup
seuser=staff_u
useradd -n -u $uid $user || exit_error "useradd failed"
semanage login -a -s $seuser $user || \
  exit_error "semanage: setting initial login user failed"

# test
expect -c "
  spawn  semanage login -m -s invalid_user_u $user 
  expect {
    -re \"libsemanage.validate_handler.*\" {exp_continue}
    -re \".*could not iterate over records$\" {exp_continue}
    -re \".*Could not modify login mapping for $user$\" {exit 0}
    -re \".*Error while renaming /etc/selinux/.*/modules/active to /etc/selinux/.*/modules/previous\.$\" {exit -2}
    -re \"\[.*\]# \" {exit -1}
  }
"

# verify
returncode=$?
if [ $returncode -ne 0 ]; then
  exit_fail "semanage set a login user that was invalid"
fi

# cleanup 
# deluser handled by tp_auth_functions

exit_pass
