#!/bin/bash
###############################################################################
# (c) Copyright Hewlett-Packard Development Company, L.P., 2006
# (c) Copyright Red Hat, Inc. All rights reserved.. 2014
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
# Verify audit of successful and unsuccessful login via su
# for local and IPA user
#


if [[ $EUID == 0 ]]; then
    source pam_functions.bash || exit 2
    source tp_ssh_functions.bash || exit 2
    backup "/etc/group"
fi

EXEC="$0 $@"

positive_test() {
    local USR=$1
    local USRPASS=$2

    if [[ $EUID == 0 ]]; then
        AUDITMARK=$(get_audit_mark)

        # setup
        # allow USR to write to tmpfile created by root
        chmod 666 $tmp1

        # turn off screen in /etc/profile
        backup /etc/profile
        sed -i 's/\[ -w $(tty) \]/false/' /etc/profile

        # test
        # rerun this script as USR.  Confine the exports to a subshell
        # We must be in group 'wheel' to execute /bin/su.
        (
            export tmp1=$tmp1
            local TEST_EUID=$(id -u "$USR")
            local WHEEL_GID=$(grep wheel /etc/group | cut -d: -f3)
            # add test user to wheel group - needed for sssd su
            usermod -aG wheel $USR
            perl -MPOSIX -e "setgid $WHEEL_GID; setuid $TEST_EUID; system \"$EXEC\""
        )

        # returned from test, collect results
        pts=$(<$tmp1)
        pts=${pts##*/}

        # check for expected audit events
        while read LINE; do
            read EVENT PAMOP GRANTORS <<< "$LINE"
            msg_1="acct=\"$USR\" exe=\"/usr/bin/su\" hostname=\? addr=\? terminal=pts/$pts res=success"
            augrok --seek $AUDITMARK type=$EVENT
            augrok --seek $AUDITMARK type=$EVENT \
                msg_1=~"op=PAM:$PAMOP grantors=$GRANTORS $msg_1" || \
                exit_fail "Successful authentication attempt not audited correctly for: $LINE"
        done <<< "$EPG"
    else
        # This is reached through the perl -e 'system...' above
        expect -c "
            spawn /bin/su - $USR
            expect {*assword:} {send $USRPASS\r}
            expect {*$USR} {send PS1=:::\r}
            expect {:::$} {send \"tty > $tmp1\r\"}
            expect {:::$} {close; wait}"
    fi
}

negative_test() {
    local USR=$1

    if [[ $EUID == 0 ]]; then
        AUDITMARK=$(get_audit_mark)

        # test -- reruns this script as USR
        # We must be in group 'wheel' to execute /bin/su.
        local TEST_EUID=$(id -u "$USR")
        local WHEEL_GID=$(grep wheel /etc/group | cut -d: -f3)
        usermod -a -G wheel $USR
        perl -MPOSIX -e "setgid $WHEEL_GID; setuid $TEST_EUID; system \"$EXEC\""

        # check for expected USER_AUTH audit event
        MSG="op=PAM:authentication grantors=\? acct=\"$USR\""
        MSG="$MSG exe=\"/usr/bin/su\" hostname=\? addr=\? terminal=pts/[0-9]+ res=failed"
        augrok --seek $AUDITMARK type=USER_AUTH
        augrok type=USER_AUTH msg_1=~"$MSG" || \
            exit_fail "Failed authentication attempt for user $USR not audited correctly"
    else
        expect -c "
            spawn /bin/su - $USR
            expect {*assword:} {send badpassword\r}
            expect {incorrect password} {close; wait}"
    fi
}

mls_test() {
    local USR=$1
    local LEVEL=$2
    local OUT=

    # remove screen and strong rng
    disable_ssh_strong_rng

    # set TEST_USER to level s5
    semanage login -a -s staff_u -r $LEVEL $TEST_USER
    append_cleanup "semanage login -d $TEST_USER"
    backup "/etc/group"
    usermod -aG wheel $TEST_USER

    # login as TEST_USER and su to root
    OUT=$(
        expect -c "
            set timeout 60
            spawn ssh ${TEST_USER}@localhost
            expect {
                {*yes/no} { send yes\r; exp_continue }
                {*assword} { send $TEST_USER_PASSWD\r }
                default { exit 1 }
            }
            expect {
                {*${TEST_USER}} { send \"newrole -r sysadm_r\r\"; exp_continue }
                {*assword} { send $TEST_USER_PASSWD\r }
                default { exit 3 }
            }
            expect {
                {*/sysadm_r} { send \"/bin/su - -c 'id -Z'; exit\r\"; exp_continue}
                {*assword} { send $PASSWD\r }
            }
            expect {*logout} { exit 0 }
            exit 4
        "
    )

    echo $OUT | grep -q staff_u:sysadm_r:sysadm_t:$LEVEL || exit_fail \
        "Unexpected context after /bin/su"
}

case $1 in
    local)
        # expected AUDIT_EVENT PAM_OPERATION GRANTOR triple
        EPG="USER_AUTH authentication pam_faillock,pam_unix
             USER_ACCT accounting pam_faillock,pam_unix,pam_localuser
             USER_START session_open pam_keyinit,pam_keyinit,pam_limits,pam_systemd,\
pam_unix,pam_xauth"
        positive_test $TEST_USER $TEST_USER_PASSWD
        ;;
    sssd)
        # expected AUDIT_EVENT PAM_OPERATION GRANTOR triple
        EPG="USER_AUTH authentication pam_faillock,pam_sss
             USER_ACCT accounting pam_faillock,pam_unix,pam_sss,pam_permit
             USER_START session_open pam_keyinit,pam_keyinit,pam_limits,\
pam_systemd,pam_unix,pam_sss,pam_xauth"
        source $TOPDIR/utils/auth-server/ipa_env
        if [[ $EUID == 0 ]]; then
            restart_service sssd
            prepend_cleanup "stop_service sssd"
        fi
        positive_test $IPA_USER $IPA_PASS
        ;;
    local_fail)
        negative_test $TEST_USER
        ;;
    sssd_fail)
        source $TOPDIR/utils/auth-server/ipa_env
        if [[ $EUID == 0 ]]; then
            restart_service sssd
            prepend_cleanup "stop_service sssd"
        fi
        negative_test $IPA_USER
        ;;
    mls)
        # check if su does not change levels
        for LEVEL in s5 s9 s3-s7; do
            mls_test $TEST_USER $LEVEL
        done
        ;;
    *)
        exit_error "Unknown testcase"
        ;;
esac

exit_pass
