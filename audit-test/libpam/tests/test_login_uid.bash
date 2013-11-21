#!/bin/bash
###############################################################################
#   Copyright (c) 2013 Red Hat, Inc. All rights reserved.
#
#   This copyrighted material is made available to anyone wishing
#   to use, modify, copy, or redistribute it subject to the terms
#   and conditions of the GNU General Public License version 2.
#
#   This program is distributed in the hope that it will be
#   useful, but WITHOUT ANY WARRANTY; without even the implied
#   warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
#   PURPOSE. See the GNU General Public License for more details.
#
#   You should have received a copy of the GNU General Public
#   License along with this program; if not, write to the Free
#   Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
#   Boston, MA 02110-1301, USA.
###############################################################################
#
# PURPOSE:
# This test checks if pam configurations for all entry point applications
# include pam_loginuid in session management group.
#
# To test the correct functionality of pam_loginuid ssh and login is used.
# The login via login command should fail as login uid is already set in the
# session. This is a new security feature introduced in this kernel commit [1].
# When connecting via ssh the login should succeed and correct USER_* events
# should be logged in audit.log.
#
# [1] https://git.kernel.org/cgit/linux/kernel/git/torvalds/linux.git/commit/?id=633b4545
#

source pam_functions.bash || exit 2
source tp_ssh_functions.bash || exit 2
disable_ssh_strong_rng

ENTRY_APPS="login sshd remote crond pluto ssh-keycat"
AUDIT_RULES="/etc/audit/audit.rules"

# In LSPP run the login in correct context
[ "$PPROFILE" == lspp ] && RUNCON="runcon -r system_r -u system_u -l s0-s15:c0.c1023"

# sysadm_u can login via ssh during testing
setsebool ssh_sysadm_login=1
append_cleanup "setsebool ssh_sysadm_login=0"

#
# check if all entry point application have pam_loginuid in PAM configurations
#
for APP in $ENTRY_APPS; do
    egrep -q "session.*required.*pam_loginuid.*" /etc/pam.d/$APP || \
        exit_fail "$APP pam configuration is missing required pam_loginuid"
done

#
# check if audit.rules contains --loginuid-immutable and immutable login set
#
egrep -q "^--loginuid-immutable" $AUDIT_RULES || \
    exit_fail "Required --loginuid-immutable not found in $AUDIT_RULES"
auditctl -s | grep -q "loginuid_immutable 1 locked" || \
    exit_fail "Immutable login not set (see auditctl -s)"

#
# test if correct auid set when logged in via ssh
#
AUDITMARK=$(get_audit_mark)
ssh_connect_pass $TEST_USER $TEST_USER_PASSWD \
        $TEST_ADMIN $TEST_ADMIN_PASSWD
auid=$(id -u $TEST_ADMIN)
msg_1="acct=\"$TEST_ADMIN\" exe=\"/usr/sbin/sshd\" hostname=localhost addr=::1 terminal=ssh res=success"
augrok --seek=$AUDITMARK -q type=USER_AUTH msg_1=~"op=PAM:authentication $msg_1" || \
    exit_fail "No correct USER_AUTH event found"
augrok --seek=$AUDITMARK -q type=USER_ACCT msg_1=~"op=PAM:accounting $msg_1" || \
    exit_fail "No correct USER_ACCT event not found"
augrok --seek=$AUDITMARK -q type=USER_START msg_1=~"op=PAM:session_open $msg_1" auid=$auid || \
    exit_fail "No correct USER_START event found"

#
# test if pam_loginuid will deny login event if the process has
# CAP_AUDIT_CONTROL capability and loginuid already set
#
# calling login in this manner leaves an entry in /var/run/utmp
# use backup (and automatic restore) to work around this
backup /var/run/utmp
(
    $RUNCON expect -c "
        set timeout 3
        spawn login
        sleep 1
        expect -nocase {login: $} {send $TEST_USER\r}
        expect -nocase {password: $} {send $TEST_USER_PASSWD\r}
        expect {
            expect -nocase {level} {send \r; exp_continue}
            {Cannot make/remove} { exit 1 }
        }
        exit 0
        "
)
[ $? -eq 0 ] && exit_fail \
    "login passed with loginuid already set in session and CAP_AUDIT_CONTROL"


#
# try to change loginuid directly for the current process
#
echo 0 | tee /proc/self/loginuid && exit_fail \
    "The loginuid is not immutable"

exit_pass

# vim: ai ts=2 sw=2 et sts=2 ft=sh
