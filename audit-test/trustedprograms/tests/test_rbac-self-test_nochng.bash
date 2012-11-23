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
# Verify that the rbac-self-test does not report any changes when there have
# not been any changes.  To perform this test first a snapshot needs to be
# taken, then a second run to show that with no changes the test completes
# successfully.

source testcase.bash || exit 2

# test
expect -c '
  set timeout -1
  spawn newrole -r sysadm_r
  expect -nocase {password: $} {send "$env(PASSWD)\r"}
  send "PS1=\"::\\#$ \"\r";
  expect {
    ::2 { send "/usr/sbin/rbac-self-test -s -v\r"; exp_continue }
    ::3 { send "/usr/sbin/rbac-self-test -v\r"; exp_continue }
    ::4 { send "exit\r"; exit 0; }
  }
  exit 1;'

# verify
if [ $? -ne 0 ]; then
  exit_error "the rbac-self-test failed"
fi

msg1="The RBAC self test succeeded.*"
augrok -q type=TEST msg_1=~"${msg1}" || \
  exit_fail "unable to find audit record containing $msg1"

exit_pass

