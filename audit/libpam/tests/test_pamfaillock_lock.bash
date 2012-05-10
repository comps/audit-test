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
# Verify pam_faillock will lock an account

source pam_functions.bash || exit 2
source tp_ssh_functions.bash || exit 2

# make sure faillock is reset for TEST_USER
/sbin/faillock --user $TEST_USER --reset > /dev/null || exit_error

# setup
tuid=$(id -u $TEST_USER)
grep -q pam_faillock /etc/pam.d/sshd || grep -q pam_faillock /etc/pam.d/password-auth || exit_error
disable_ssh_strong_rng

# Unlike pam_tally2, faillock doesn't have a --reset=n option that lets us 
# pre-set the number of failures. So we need to fail the login multiple times 
# until we reach the deny limit. When this test was written, a RHEL6.1 
# evaluation system required three failures to trigger a lockout. YMMV.

expect -c '
    spawn ssh $env(TEST_USER)@localhost
    expect {
        {continue} { send "yes\r"; exp_continue }
    {assword} { send "badpassword\r" }
    }
    expect {
        {denied} { exp_continue }
        {assword} { send "badpassword\r" }
    }
    expect {
        {denied} { exp_continue }
        {assword} { send "badpassword\r" }
    }
    expect -nocase {denied} { close; wait }'

# test
msg_1="pam_faillock uid=$tuid.*exe=./usr/sbin/sshd.*res=success.*"
augrok -q type=ANOM_LOGIN_FAILURES msg_1=~"$msg_1" || exit_fail
augrok -q type=RESP_ACCT_LOCK msg_1=~"$msg_1" || exit_fail

# clean up
/sbin/faillock --user $TEST_USER --reset > /dev/null || exit_error

exit_pass
