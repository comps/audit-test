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
# Verify pam_faillock will unlock an account

source pam_functions.bash || exit 2
source tp_ssh_functions.bash || exit 2

# setup
tuid=$(id -u $TEST_USER)
grep -q pam_faillock /etc/pam.d/sshd || grep -q pam_faillock /etc/pam.d/password-auth || \
    exit_error "pam_faillock does not seem to be configured in PAM"
disable_ssh_strong_rng

AUDITMARK=$(get_audit_mark)

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
/sbin/faillock --user $TEST_USER --reset > /dev/null || \
    exit_error "faillock reset for $TEST_USER user failed"

augrok --seek=$AUDITMARK type=USER_MGMT
MSG="op=faillock-reset id=$tuid exe=\"/usr/sbin/faillock\" hostname=$(hostname) addr=\? terminal=pts/[0-9]+ res=success"
augrok --seek=$AUDITMARK -q type=USER_MGMT msg_1=~"$MSG" || \
    exit_fail "USER_MGMT failock-reset message not found"

# verify the account is unlocked
expect -c '
    spawn ssh $env(TEST_USER)@localhost
    expect {
        {continue} { send "yes\r"; exp_continue }
        {assword} {
            send "$env(TEST_USER_PASSWD)\r"
            send "PS1=:\\::\r"
        }
    }
    expect {:::$} { send "exit\r"; close; wait }'

MSG="acct=\"$TEST_USER\" exe=./usr/sbin/sshd.*terminal=ssh res=success.*"
augrok --seek=$AUDITMARK type=CRED_ACQ
augrok --seek=$AUDITMARK type=CRED_ACQ msg_1=~"PAM:setcred grantors=pam_env,pam_faillock,pam_unix $MSG" || \
    exit_fail "Expected CRED_ACQ message not found"
augrok --seek=$AUDITMARK type=CRED_ACCT
augrok --seek=$AUDITMARK type=USER_ACCT msg_1=~"PAM:accounting grantors=pam_faillock,pam_unix,pam_localuser $MSG" || \
    exit_fail "Expected USER_ACCT mesage not found"

exit_pass
