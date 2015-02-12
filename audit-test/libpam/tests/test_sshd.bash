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
# SFRs: FIA_USB.2
#
# PURPOSE:
# Verify audit of successful ssh. Also check if all uids are correct after login.
#

source pam_functions.bash || exit 2
source tp_ssh_functions.bash || exit 2
disable_ssh_strong_rng

# audit mark for seeking
AUDITMARK=$(get_audit_mark)

# in MLS mode check also for pam_namespace success
[ "$PPROFILE" = "lspp" ] && PAM_ADDTL="pam_namespace,"

# add TEST_USER to a test group
groupadd SUPGROUP
gpasswd -a $TEST_USER SUPGROUP
prepend_cleanup "groupdel SUPGROUP"

# test
LOGINLOG=$(
    expect -c "
        spawn ssh ${TEST_USER}@localhost
        expect {
            {*continue} {send yes\r; exp_continue}
            {*assword} {send ${TEST_USER_PASSWD}\r}
        }
        expect {*$TEST_USER} {send \"ps -eo ruid,euid,fsuid,rgid,egid,fsgid --no-headers -q \$\$\r\"}
        expect {*$TEST_USER} {send \"groups\r\"}
        expect {*$TEST_USER} {send \"exit\r\"}
        expect eof
        exit 0"
)

# FIA_USB.2
# check if all ruid,euid,fsuid,rgid,egid,fsgid correct after login
TUID=$(id -u $TEST_USER)
TGID=$(id -g $TEST_USER)
echo "$LOGINLOG" | egrep "([[:space:]]*$TUID){3}([[:space:]]*$TGID){3}" || \
    exit_fail "Unexpected UIDs after login"
# check if user in correct groups after login
TGROUPS=$(groups $TEST_USER | sed "s/$TEST_USER : //")
echo "$LOGINLOG" | egrep "$TGROUPS" || \
    exit_fail "Unexpected groups after login"

# expected audit events
DATA="USER_AUTH authentication pam_faillock,pam_unix
      USER_ACCT accounting pam_faillock,pam_unix,pam_localuser
      CRED_ACQ setcred pam_env,pam_faillock,pam_unix
      USER_START session_open pam_selinux,pam_loginuid,pam_selinux,\
${PAM_ADDTL}pam_keyinit,pam_keyinit,pam_limits,pam_systemd,pam_unix,pam_lastlog
      CRED_ACQ setcred pam_env,pam_faillock,pam_unix
      USER_END session_close pam_selinux,pam_loginuid,pam_selinux,\
${PAM_ADDTL}pam_keyinit,pam_keyinit,pam_limits,pam_systemd,pam_unix,pam_lastlog
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
