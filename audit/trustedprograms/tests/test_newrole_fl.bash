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
# Verify that newrole -l fails when attempting to change to a level above your
# current range.  Use runcon to start an expect session at SystemLow-Secret
# then inside that attempt to newrole to SystemHigh-SystemHigh
# /etc/selinux/mls/contexts/securetty_types needs to be modified in order to
# allow this test to work since ptys are disallowed to change levels by default

source testcase.bash || exit 2

# setup
backup /etc/selinux/mls/contexts/securetty_types
tty_type=$(ls -lZ $(tty) | awk -F: '{print $3}')
echo $tty_type >> /etc/selinux/mls/contexts/securetty_types

# test
runcon -l SystemLow-Secret -- expect -c "
  spawn  newrole -l SystemHigh-SystemHigh
  expect {
    -nocase Authenticating {exp_continue}
    -nocase \"Password: \" {send \"$PASSWD\\r\"; exp_continue}
    -re \"Error!  Could not set new context for .*$\" {exit 0}
    -re \".*is not a valid context$\" {exit -2}
    -re \"\[.*\]# \" {exit -1}
  }
"

# verify
returncode=$?
if [ $returncode -eq 255 ]; then
  exit_fail "newrole changed to a level that should have been inaccessible"
fi

if [ $returncode -ne 0 ]; then
  exit_fail "an unexpected error occured during the newrole"
fi

exit_pass
