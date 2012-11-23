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
# Verify pam_tally2 will lock an account

source pam_functions.bash || exit 2

# setup
tuid=$(id -u $TEST_USER)
grep -q pam_tally2 /etc/pam.d/sshd || exit_error
/sbin/pam_tally2 -u $TEST_USER --reset=4 --quiet || exit_error

expect -c '
    spawn ssh $env(TEST_USER)@localhost
    expect -nocase {Are you sure you want to continue} {send "yes\r"}
    expect -nocase {password: $} {send "badpassword\r"}
    expect -nocase {permission denied}
    expect -nocase {password: $} {send "badpassword\r"}
    expect -nocase {permission denied} {close; wait}'

# test
msg_1="pam_tally2 uid=$tuid : exe=./usr/sbin/sshd.*res=success.*"
augrok -q type=ANOM_LOGIN_FAILURES msg_1=~"$msg_1" || exit_fail
augrok -q type=RESP_ACCT_LOCK msg_1=~"$msg_1" || exit_fail

# clean up
/sbin/pam_tally2 -u $TEST_USER -r --quiet || exit_error

exit_pass
