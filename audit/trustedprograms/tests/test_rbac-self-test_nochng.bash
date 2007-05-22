#!/bin/bash
###############################################################################
# (c) Copyright Hewlett-Packard Development Company, L.P., 2007
#
#   This program is free software;  you can redistribute it and/or modify
#   it under the terms of the GNU General Public License as published by
#   the Free Software Foundation; either version 2 of the License, or
#   (at your option) any later version.
#
#   This program is distributed in the hope that it will be useful,
#   but WITHOUT ANY WARRANTY;  without even the implied warranty of
#   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See
#   the GNU General Public License for more details.
#
#   You should have received a copy of the GNU General Public License
#   along with this program;  if not, write to the Free Software
#   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
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
  set timeout 90
  spawn newrole -r sysadm_r
  expect -nocase {password: $} {send "$env(PASSWD)\r"}
  send "PS1=\"::\\#$ \"\r";
  expect {
    ::2 { send "/usr/sbin/rbac-self-test -s -v\r"; exp_continue }
    ::3 { send "/usr/sbin/rbac-self-test -v\r"; exp_continue }
    ::4 { send "exit\r"; exit; }
  } '

# verify
if [ $? -ne 0 ]; then
  exit_fail "the rbac-self-test did not complete its check run correctly"
fi

msg1="The RBAC self test succeeded.*"
augrok -q type=TEST msg_1=~"${msg1}" || \
  exit_fail "unable to find audit record containing $msg1"

exit_pass

