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
# Verify audit of successful ssh.

source pam_functions.bash || exit 2
source tp_ssh_functions.bash || exit 2
disable_ssh_strong_rng

# audit mark for seeking
AUDITMARK=$(get_audit_mark)

# test
expect -c "
    spawn ssh ${TEST_USER}@localhost
    expect {
        {continue} {send yes\r; exp_continue}
        {assword} {send ${TEST_USER_PASSWD}\r}
    }
    expect {$TEST_USER} {send exit\r}
    expect eof
    exit 0"

DATA="USER_AUTH authentication pam_faillock,pam_unix
      USER_ACCT accounting pam_faillock,pam_unix,pam_localuser
      CRED_ACQ setcred pam_env,pam_faillock,pam_unix
      USER_START session_open pam_selinux,pam_loginuid,pam_selinux,\
pam_namespace,pam_keyinit,pam_keyinit,pam_limits,pam_systemd,pam_unix,pam_lastlog
      CRED_ACQ setcred pam_env,pam_faillock,pam_unix
      USER_END session_close pam_selinux,pam_loginuid,pam_selinux,\
pam_namespace,pam_keyinit,pam_keyinit,pam_limits,pam_systemd,pam_unix,pam_lastlog
      CRED_DISP setcred pam_env,pam_faillock,pam_unix"

while read LINE; do
    [ -z "$LINE" ] && break
    read EVENT PAMTYPE GRANTORS <<< "$LINE"
    augrok --seek=$AUDITMARK type==$EVENT
    augrok --seek=$AUDITMARK type==$EVENT msg_1="op=PAM:$PAMTYPE \
grantors=$GRANTORS acct=\"$TEST_USER\" exe=\"/usr/sbin/sshd\" hostname=localhost \
addr=::1 terminal=ssh res=success" || exit_fail \
        "Expected $EVENT audit event for $TEST_USER successful login not found"
done <<< "$DATA"

exit_pass
