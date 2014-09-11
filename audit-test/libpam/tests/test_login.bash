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
# Verify audit of successful and failed console login for local and IPA user.
#
# SFRs: FIA_UAU.1(HU)
#

source pam_functions.bash || exit 2

do_login() {
    local PTS=
    local TUSR=$1
    local TPASS=$2

    # test
    (
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
            expect {:::$} {close; wait}"
    )

    PTS=$(<$localtmp)
    PTS=${PTS##*/}

    msg_1="acct=\"*$TUSR\"*[ :]* exe=.*/bin/login.* terminal=.*pts/$PTS res=success.*"
    augrok -q type=USER_AUTH msg_1=~"PAM: *authentication $msg_1" || exit_fail
    augrok -q type=USER_ACCT msg_1=~"PAM: *accounting $msg_1" || exit_fail
    augrok -q type=USER_START msg_1=~"PAM: *session[_ ]open $msg_1" || exit_fail
}

do_login_fail() {
    local TUSR=$1 MSG=

    expect -c "
        spawn login
        sleep 1
        expect -nocase {login: $} {send $TUSR\r}
        expect -nocase {password: $} {send badpassword\r}
        expect -nocase {login incorrect} {close; wait}"

    MSG="op=PAM:.*authentication acct=\"*$TUSR\"*[ :]*"
    MSG="$MSG exe=./usr/bin/login. .* terminal=.*pts/.*res=failed.*"
    augrok -q type=USER_AUTH msg_1=~"$MSG" || \
        exit_fail "Failed authentication attempt for user $TUSR not audited"
}

test_local() {
    # if in LSPP mode, map the TEST_USER to staff_u
    if [[ $PPROFILE == "lspp" ]]; then
        semanage login -d $TEST_USER
        semanage login -a -s staff_u $TEST_USER
        append_cleanup user_cleanup
    fi

    do_login $TEST_USER $TEST_USER_PASSWD
    do_login_fail $TEST_USER
}

test_sssd() {
    local USR=

    # start sssd
    restart_service "sssd"
    prepend_cleanup "stop_service sssd"

    # source IPA env
    . $TOPDIR/utils/auth-server/ipa_env
    local TEST_USERS="$IPA_STAFF $IPA_USER"
    [ "$PPROFILE" == "capp" ] && TEST_USERS="$IPA_STAFF"

    for USR in $TEST_USERS; do
        do_login $USR $IPA_PASS
        do_login_fail $USR
    done
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
# if loginuid already set # for the purpose of this test we disable
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
