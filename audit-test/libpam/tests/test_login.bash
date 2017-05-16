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
# This test contains multiple tests. The tests are run according to the first
# passed parameter ($1). The tests are defined by the functions test_$1.
# The tests use expect(1) to simulate user behavior on the console login
# screen.
#
# Test 'local':
#   For a local user:
#   - verify audit of successful and failed console login
#   - for successful console login verify all UIDs are correctly
#   - for successful console login verify that user is in correct groups
#
# Test 'sssd':
#   For IPA user(s):
#   - verify audit of successful and failed console login
#   - for successful console login verify all UIDs are correctly
#   - for successful console login verify that user is in correct groups
#
# Test 'banner':
#   This test checks if banner text is displayed during login. The login screen
#   is simulated by running agetty manually in a screen.
#
# Test 'feedback':
#   This test checks if password feedback is obscured (i.e. not printed
#   to the screen)
#
# DEBUGGING:
# In case of debugging it is advised to try the failed cases manually
# on an ordinary console prompt. If that works, continue with manually
# running the expect commands.
#
# SFRs:
# FIA_UAU.1(HU), FIA_USB.2
#

source pam_functions.bash || exit 2


#
# Positive test
#
do_login() {
    local PTS= LINE= EVENT= GRANTORS= PAMOP= LOGINLOG=
    local TUSR=$1
    local TPASS=$2

    # add TUSR to a test group
    grep -q SUPGROUP /etc/group || groupadd SUPGROUP
    gpasswd -a $TUSR SUPGROUP
    prepend_cleanup "groupdel SUPGROUP"

    # test
    LOGINLOG=$(
        runcon -u system_u -r system_r expect -c "
            set timeout 20
            spawn login
            sleep 1
            expect -nocase {login: $} {send $TUSR\r}
            expect -nocase {password: $} {send $TPASS\r}
            expect {
                -nocase {level} {send \r}
                {$TUSR@} {send \r}
            }
            send \"PS1=:\\::\r\"
            expect {:::$} {send \"tty > $localtmp\r\"}
            expect {:::$} {send \"ps -eo ruid,euid,fsuid,rgid,egid,fsgid --no-headers -q \$\$\r\"}
            expect {:::$} {send \"groups\r\"}
            expect {:::$} {send \"exit\r\"}
            expect eof
            close; wait"
    )

    # extract the tty number saved during the login
    # this needs to match with the audited tty
    PTS=$(<$localtmp)
    PTS=${PTS##*/}

    # FIA_USB.2
    # check if all ruid,euid,fsuid,rgid,egid,fsgid correct after login
    TUID=$(id -u $TUSR)
    TGID=$(id -g $TUSR)
    echo "$LOGINLOG" | egrep "([[:space:]]*$TUID){3}([[:space:]]*$TGID){3}" || \
        exit_fail "Unexpected UIDs after login"
    # check if user in correct groups after login
    TGROUPS=$(groups $TUSR | sed "s/$TUSR : //")
    echo "$LOGINLOG" | egrep "$TGROUPS" || \
        exit_fail "Unexpected groups after login"

    # check for correct audit events
    while read LINE; do
        read EVENT PAMOP GRANTORS <<< "$LINE"
        msg_1="acct=\"$TUSR\" exe=\"/usr/bin/login\" hostname=$(hostname) addr=\? terminal=pts/$PTS res=success"
        augrok --seek $AUDITMARK type=$EVENT
        augrok --seek $AUDITMARK type=$EVENT \
            msg_1=~"op=PAM:$PAMOP (grantors=$GRANTORS )?$msg_1" || \
            exit_fail "Successful authentication attempt not audited correctly for: $LINE"
    done <<< "$EGP"
}

#
# Negative test
#
do_login_fail() {
    local TUSR=$1 MSG=

    # Try an invalid login
    # sleep 1 is required because login forks here and expect might fail to read
    # https://github.com/karelzak/util-linux/blob/master/login-utils/login.c
    expect -c "
        spawn login
        sleep 1
        expect -nocase {login: $} {send $TUSR\r}
        expect -nocase {password: $} {send badpassword\r}
        expect -nocase {login incorrect} {close; wait}"

    # check for correct audit record via augrok
    MSG="op=PAM:authentication (grantors=\? )?acct=\"$TUSR\""
    MSG="$MSG exe=\"/usr/bin/login\" hostname=$(hostname) addr=\? terminal=pts/[0-9]+ res=failed"
    augrok --seek $AUDITMARK type=USER_AUTH
    augrok type=USER_AUTH msg_1=~"$MSG" || \
        exit_fail "Failed authentication attempt for user $TUSR not audited correctly"
}

#
# Test with local user
#
test_local() {
    # if in LSPP mode (SELinux MLS), map the TEST_USER to staff_u
    if [[ $PPROFILE == "lspp" ]]; then
        semanage login -d $TEST_USER
        semanage login -a -s staff_u $TEST_USER
        append_cleanup user_cleanup
    fi

    # triple of expected values of - audit_event pam_operation grantors field
    EGP="USER_AUTH authentication pam_securetty,pam_faillock,pam_unix
         USER_ACCT accounting pam_faillock,pam_unix,pam_localuser
         USER_START session_open pam_selinux,pam_console,pam_selinux,\
pam_namespace,pam_keyinit,pam_keyinit,pam_limits,pam_systemd,pam_unix,pam_lastlog"

    # positive test
    AUDITMARK=$(get_audit_mark)
    do_login $TEST_USER $TEST_USER_PASSWD

    # negative test
    AUDITMARK=$(get_audit_mark)
    do_login_fail $TEST_USER
}

#
# Test with IPA user(s)
#
test_sssd() {
    local USR=

    # make sure strong rng is disabled
    sssd_disable_strong_rng

    # start sssd
    restart_service "sssd"
    prepend_cleanup "stop_service sssd"

    # triple of expected values of - audit_event pam_operation grantors field
    EGP="USER_AUTH authentication pam_securetty,pam_faillock,pam_sss
         USER_ACCT accounting pam_faillock,pam_unix,pam_sss,pam_permit
         USER_START session_open pam_selinux,pam_console,pam_selinux,\
pam_namespace,pam_keyinit,pam_keyinit,pam_limits,pam_systemd,pam_unix,\
pam_sss,pam_lastlog"

    # source IPA env
    . $TOPDIR/utils/auth-server/ipa_env
    local TEST_USERS="$IPA_STAFF $IPA_USER"

    # in CAPP use only staff user for testing as they behave the same
    [ "$PPROFILE" == "capp" ] && TEST_USERS="$IPA_STAFF"

    for USR in $TEST_USERS; do
        # get audit mark
        AUDITMARK=$(get_audit_mark)
        do_login $USR $IPA_PASS
        # get audit mark
        AUDITMARK=$(get_audit_mark)
        do_login_fail $USR
    done
}

#
# Test if banner text displayed for local console login
#
test_banner() {
    local BANNER_TXT="This is a disclaimer"
    local EXP_OUT=

    # add test banner
    backup /etc/issue
    echo "$BANNER_TXT" >> /etc/issue

    # in MLS simulate as run by staff_u sysadm_t, so that .screen
    # gets labeled correctly
    [ "$PPROFILE" = "lspp" ] && \
        RUNCON="runcon staff_u:sysadm_r:sysadm_t:SystemLow-SystemHigh bash -c"
    # banner is displayed via agetty, run it in screen as it hijacks
    # the given (current) tty
    EXP_OUT=$(
        expect -c "
            set timeout 2
            spawn $RUNCON screen
            expect {
                {*root} { send \"/sbin/agetty --noclear 9600 \\\$(sed 's|/dev/||' <<< \\\$(tty))\r\" }
                timeout { exit 1 }
            }
            expect {
                {*login:} { send noone\r }
                timeout { exit 2 }
            }
            exit 0"
    ); RET=$?

    # check if expect finished as expected and banner text displayed at login
    [ $RET -eq 0 ] || exit_fail "Expect failed unexpectedly ($RET)"
    echo "$EXP_OUT" | egrep -q "^$BANNER_TXT" || exit_fail "Banner text '$BANNER_TXT' not found"
}

#
# Test if entered password is obscured on the login screen
#
test_feedback() {
    local EXP_OUT=$(
        expect -c "
            spawn login
            sleep 1
            expect -nocase {login: $} {send $TUSR\r}
            expect -nocase {password: $} {send badpassword\r}
            expect -nocase {login incorrect} {close; wait}" | xxd -c256
    )

    # check if password feedback obscured -> no output
    echo "$EXP_OUT" | egrep -q "Password: ..Login incorrect" || \
        exit_fail "Unexpected obscured output after password prompt"
}

## common setup
# turn off screen in /etc/profile
backup /etc/profile
sed -i 's/\[ -w $(tty) \]/false/' /etc/profile

# allow test userts to write to tmpfile
chmod 666 $localtmp
chcon -t user_home_t $localtmp

# calling login in this manner leaves an entry in /var/run/utmp
# use backup (and automatic restore) to work around this
backup /var/run/utmp

# In RHEL7 CC evaluated configuration the pam_loginuid fails
# if loginuid already set. For the purpose of this test we disable
# it temporarily
backup /etc/pam.d/login
sed -i 's/\(^session.*pam_loginuid.*$\)/\#\1/' /etc/pam.d/login

# choose test
if [ "$(type -t test_$1)" = "function" ]; then
    eval test_$1
else
    exit_error "Unknown test or no test given"
fi

exit_pass
