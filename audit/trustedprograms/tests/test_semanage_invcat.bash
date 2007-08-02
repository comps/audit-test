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
# Attempt to set an invalid range for an SELinux user with semanage
# verify that semanage does not allow this operation and that it errors
# correctly when the operation is attempted.

source testcase.bash || exit 2

# setup
seuser=user_u
inv_range=s0-s1.foo

# test
expect -c "
  spawn  semanage user -m -r $inv_range $seuser 
  expect {
    -re \".*invalid MLS context ${inv_range}$\" {exp_continue}
    -re \".*could not construct mls context structure$\" {exp_continue}
    -re \"lib.*_modify:could not.*\" {exp_continue}
    -re \".*Could not modify SELinux user ${seuser}$\" {exit 0}
    -re \".*Error while renaming /etc/selinux/.*/modules/active to /etc/selinux/.*/modules/previous\.$\" {exit -2}
    -re \"\[.*\]# \" {exit -1}
  }
"

# verify
returncode=$?
if [ $returncode -ne 0 ]; then
  exit_fail "semanage allowed setting an invalid range"
fi

# cleanup 

exit_pass
