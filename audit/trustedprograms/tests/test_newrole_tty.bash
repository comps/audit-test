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
# Verify that newrole -l fails when the tty type is not listed in
# /etc/selinux/mls/contexts/securetty_types  The test first checks to be sure
# that the terminal type is not in that database, and then attempts to change
# levels

source testcase.bash || exit 2
set -x

# setup
tty_type=$(ls -lZ $(tty) | awk -F: '{print $3}')
grep -q $tty_type /etc/selinux/mls/contexts/securetty_types \
	&& exit_fail "$tty_type appears in /etc/selinux/mls/contexts/securetty_types"

# test
expect -c "
  spawn  newrole -l SystemHigh
  expect {
    Authenticating {exit -1}
    -re \".*not allowed to change levels on a non secure terminal\" {exit 0}
  }"

# verify
if [ $? -ne 0 ]; then
  exit_fail "newrole allowed changing levels on an insecure terminal"
fi

exit_pass
