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
# This test checks that all tools, which are able to faciliatate unix acc
# password change, conform to password rules specified by pam_pwquality.so
# (and, by extension, pam_cracklib.so) and pam_pwhistory.so.
#

#
# users, passwords
#

#TEST_USER="$TEST_USER"
TEST_UID="$(id -u "$TEST_USER")"
TEST_GID="$(grep wheel /etc/group | cut -f3 -d:)"
[ "$TEST_USER" -a "$TEST_UID" -a "$TEST_GID" ] || exit 2

# sample set of passwords that should match the required rules
PASSWD_VALID='b456jgt3,adf kHSf4yH7* 175@0.42 §356-OP9A SFIMNBXP5%OPW oqkfpbe1*'

# sample set of passwords that shouldn't match the rules
# (first password is 6 changes away from last valid usage,
#  it should therefore fail with pam_pwhistory set to 6)
PASSWD_INVALID='b456jgt3,adf afaiojfiud dghsdgsdd123 aA1$ example1*'

# PAM grantors in audit events - note that these are sensitive
# to the PAM stack configuration
GRANTORS="pam_pwquality,pam_pwhistory,pam_unix"

#
# helper functions
#

clear_pwquality_conf()
{
    echo -n > /etc/security/pwquality.conf || \
        exit_error "unable to truncate pwquality.conf"
}
run_user()
{
    run-as.py -u "$TEST_UID" -g "$TEST_GID" "$@"
    return $?
}
can_login()
{
    # $1 = username, $2 = password
    [ -z "$1" -o -z "$2" ] && return 1
    faillock --user "$1" --reset
    runcon -u system_u -r system_r expect -c "
spawn login
expect_before default { exit 1 }
sleep 1
expect {*login: } { send -- \"$1\r\" }
expect {Password: } { send -- \"$2\r\" }
expect {
    {*(current) UNIX password: } { exit 1 }
    {*New password: } { exit 1 }
    {*Login incorrect} { exit 1 }
    {*Would you like to enter a different role or level} { exit 0 }
    {*$1@} { exit 0 }
}
exit 1
"
    return $?
}

#
# rule modification access testing functions
#
# retval == 0 (true): was able to perform modification
# retval == 1 (false): was not able to perform modification
#

# modify pwquality.conf directly
rules_pwquality_conf()
{
    echo 'minlen = 124' >> /etc/security/pwquality.conf
    grep '^minlen = 124$' /etc/security/pwquality.conf >/dev/null \
        && return 0 || return 1
}

# modify pam.d/system-auth and pam.d/password-auth directly
rules_pamd_auth()
{
    for file in /etc/pam.d/system-auth /etc/pam.d/password-auth; do
        grep 'pam_pwquality.so' >/dev/null "$file" || \
            exit_error "unexpected: missing pam_pwquality.so in $file"
        sed -i '/pam_pwquality.so/s/$/ minlen=125/' "$file"
        # success for at least one of the files counts as success
        grep 'pam_pwquality.so.*minlen=125' "$file" && return 0
    done
    return 1
}

#
# testing functions
#
# *_auth:
#   $1 = tested (new) password
#   retval == 0: password change succeeded
#   retval == 1: password change failed due to exhausted retries (bad password)
#   retval == 2: anything else - unexpected error occured
#
# *_audit_{pass,fail}:
#   $1 = audit mark
#   retval == 0 (true): audit log entry found
#   retval == 1 (false): audit log entry not found
#

# passwd (/bin/passwd)
passwd_auth()
{
    expect -c "
exp_internal 1
set timeout 30
spawn passwd
expect_before default { exit 2 }
expect {*(current) UNIX password: } { send -- \"$TEST_OLDPASS\r\" }
expect {
    {*New password: } { send -- \"$1\r\"; exp_continue }
    {*Retype new password: } { send -- \"$1\r\"; exp_continue }
    {*updated successfully} { exit 0 }
    {*exhausted maximum number of retries} { exit 1 }
    {*error} { exit 2 }
}
exit 2
"
    return $?
}
passwd_audit_pass()
{
    augrok --seek "$1" type==USER_CHAUTHTOK
    augrok -q --seek "$1" type==USER_CHAUTHTOK msg_1=~"op=PAM:chauthtok grantors=$GRANTORS acct=.$TEST_USER. exe=./usr/bin/passwd..*res=success"
    return $?
}
passwd_audit_fail()
{
    augrok --seek "$1" type==USER_CHAUTHTOK
    augrok -q --seek "$1" type==USER_CHAUTHTOK msg_1=~"op=PAM:chauthtok grantors=\? acct=.$TEST_USER. exe=./usr/bin/passwd..*res=failed"
    return $?
}

# su (/bin/su)
su_auth()
{
    expect -c "
exp_internal 1
set timeout 30
spawn /bin/su -c \"echo test logged in\" - $TEST_USER
expect_before default { exit 2 }
expect {Password: } { send -- \"$TEST_OLDPASS\r\" }
expect {*(current) UNIX password: } { send -- \"$TEST_OLDPASS\r\" }
expect {
    {*New password: } { send -- \"$1\r\"; exp_continue }
    {*Retype new password: } { send -- \"$1\r\"; exp_continue }
    {*test logged in} { exit 0 }
    {*exhausted maximum number of retries} { exit 1 }
    {*error} { exit 2 }
}
exit 2
"
    return $?
}
su_audit_pass()
{
    augrok --seek "$1" type==USER_CHAUTHTOK
    augrok -q --seek "$1" type==USER_CHAUTHTOK msg_1=~"op=PAM:chauthtok grantors=$GRANTORS acct=.$TEST_USER. exe=./usr/bin/su..*res=success"
    return $?
}
su_audit_fail()
{
    augrok --seek "$1" type==USER_CHAUTHTOK
    augrok -q --seek "$1" type==USER_CHAUTHTOK msg_1=~"op=PAM:chauthtok grantors=\? acct=.$TEST_USER. exe=./usr/bin/su..*res=failed"
    return $?
}

# login (/bin/login)
login_auth()
{
    # we need to launch login(1) the way init would have exec'd it
    # and wait one extra second for it to initialize
    runcon -u system_u -r system_r expect -c "
exp_internal 1
set timeout 30
spawn login
expect_before default { exit 2 }
sleep 1
expect {*login: } { send -- \"$TEST_USER\r\" }
expect {Password: } { send -- \"$TEST_OLDPASS\r\" }
expect {*(current) UNIX password: } { send -- \"$TEST_OLDPASS\r\" }
expect {
    {*New password: } { send -- \"$1\r\"; exp_continue }
    {*Retype new password: } { send -- \"$1\r\"; exp_continue }
    {*Would you like to enter a different role or level} { exit 0 }
    {*$TEST_USER@} { exit 0 }
    {*exhausted maximum number of retries} { exit 1 }
    {*error} { exit 2 }
}
exit 2
"
    return $?
}
login_audit_pass()
{
    augrok --seek "$1" type==USER_CHAUTHTOK
    augrok -q --seek "$1" type==USER_CHAUTHTOK msg_1=~"op=PAM:chauthtok grantors=$GRANTORS acct=.$TEST_USER. exe=./usr/bin/login..*res=success"
    return $?
}
login_audit_fail()
{
    augrok --seek "$1" type==USER_CHAUTHTOK
    augrok -q --seek "$1" type==USER_CHAUTHTOK msg_1=~"op=PAM:chauthtok grantors=\? acct=.$TEST_USER. exe=./usr/bin/login..*res=failed"
    return $?
}

# ssh (/usr/sbin/sshd)
ssh_auth()
{
    # ssh is a special case - with
    #   ChallengeResponseAuthentication no
    #   PasswordAuthentication yes
    # it treats failed password change like a failed auth attempt,
    # so it would go 3x3 times before giving up, so we limit it to just
    # one auth attempt, eg. one PAM round with 3 pw change attempts
    expect -c "
exp_internal 1
set timeout 60
spawn ssh -t -o StrictHostKeyChecking=no -o NumberOfPasswordPrompts=1 $TEST_USER@localhost \"echo test logged in\"
expect_before default { exit 2 }
expect {
    {$TEST_USER@localhost's password: } { send -- \"$TEST_OLDPASS\r\" }
    {Password: } { send -- \"$TEST_OLDPASS\r\" }
}
expect {*(current) UNIX password: } { send -- \"$TEST_OLDPASS\r\" }
expect {
    {*New password: } { send -- \"$1\r\"; exp_continue }
    {*Password: } { send -- \"$1\r\"; exp_continue }
    {*Retype new password: } { send -- \"$1\r\"; exp_continue }
    {*test logged in} { exit 0 }
    {*Permission denied} { exit 1 }
    {*error} { exit 2 }
}
exit 2
"
    return $?
}
ssh_audit_pass()
{
    augrok --seek "$1" type==USER_CHAUTHTOK
    augrok -q --seek "$1" type==USER_CHAUTHTOK msg_1=~"op=PAM:chauthtok grantors=$GRANTORS acct=.$TEST_USER. exe=./usr/sbin/sshd..*res=success"
    return $?
}
ssh_audit_fail()
{
    augrok --seek "$1" type==USER_CHAUTHTOK
    augrok -q --seek "$1" type==USER_CHAUTHTOK msg_1=~"op=PAM:chauthtok grantors=\? acct=.$TEST_USER. exe=./usr/sbin/sshd..*res=failed"
    return $?
}

#
# main, test management (mostly root) / parent part
#

if [ -z "$PARENT_SETUP" ]; then
    # source these only once and only in the parent,
    # to avoid backup buffer reset
    source pam_functions.bash || exit 2
    source tp_ssh_functions.bash || exit 2
    export PARENT_SETUP=1

    backup /etc/security/pwquality.conf
    backup /etc/security/opasswd
    backup /etc/pam.d/system-auth
    backup /etc/pam.d/password-auth
    # in case authconfig is installed (the above would just backup symlinks)
    [ -f /etc/pam.d/system-auth-ac ] && backup /etc/pam.d/system-auth-ac
    [ -f /etc/pam.d/password-auth-ac ] && backup /etc/pam.d/password-auth-ac
    disable_ssh_strong_rng

    # In RHEL7 CC evaluated configuration the pam_loginuid fails
    # if loginuid is already set - for the purpose of this test, we disable
    # it temporarily
    backup /etc/pam.d/login
    sed -i 's/\(^session.*pam_loginuid.*$\)/\#\1/' /etc/pam.d/login

    case "$1" in
        # try to alter the password rules (sanity test)
        'alter_rules')
            if [ "$2" = "root" ]; then
                rules_pwquality_conf || \
                    exit_fail "root was unable to alter pwquality.conf"
                rules_pamd_auth || \
                    exit_fail "root was unable to change pam.d/*-auth"
            elif [ "$2" = "user" ]; then
                # re-execute and check as user
                run_user "$0" "$1" "$2"
                case "$?" in
                    0) exit_fail "user was able to alter password rules" ;;
                    2) exit_error "error in user section of rule alteration" ;;
                esac
            else
                exit_error "unknown option for alter_rules"
            fi
            ;;
        'try_auth')
            [ $# -lt 2 ] && exit_error "missing argument for try_auth"

            # initialize test user's password to some initial value,
            # TEST_OLDPASS is set after every password change for the next
            # password change
            # - we do not use 'echo $user:oldpass | chpasswd' before
            #   every test, because it alters pwhistory and invalidates
            #   the test
            # - use export to propagate it to the spawned child process
            export TEST_OLDPASS="origpass"
            echo "$TEST_USER:$TEST_OLDPASS" | chpasswd || exit_error

            # restore TEST_USER password after testing
            append_cleanup "echo \"$TEST_USER:$TEST_USER_PASSWD\" | chpasswd"

            # reset faillock before exiting, to prevent further tests from
            # failing
            append_cleanup "faillock --user \"$TEST_USER\" --reset"

            # clean testing environment
            echo -n > /etc/security/pwquality.conf
            echo -n > /etc/security/opasswd

            # set up the required minimal rules specified by the ST
            # - min password length is 8 chars
            # - at least 1 number, at least 1 special char
            echo "minlen = 8" >> /etc/security/pwquality.conf
            echo "dcredit = -1" >> /etc/security/pwquality.conf
            echo "ocredit = -1" >> /etc/security/pwquality.conf
            # - history of last 6 used passwords that cannot be used as new
            for file in /etc/pam.d/system-auth /etc/pam.d/password-auth; do
                # insert pam_pwhistory before pam_unix
                sed -i '/password\s*sufficient\s*pam_unix.so/ipassword    requisite     pam_pwhistory.so remember=6 use_authtok' "$file"
            done

            # valid passwords
            for password in $PASSWD_VALID; do
                # expire current password
                passwd -e "$TEST_USER"
                faillock --user "$TEST_USER" --reset
                auditmark=$(get_audit_mark)

                # execute child part with further testing logic
                # (run as user, except for login(1) test, which needs root)
                if [ "$2" = "login" ]; then
                    "$0" "$1" "$2" "$password"
                else
                    run_user "$0" "$1" "$2" "$password"
                fi
                case "$?" in
                    1) exit_fail "user was unable to specify a valid password" ;;
                    2) exit_error "error when testing interfaces for valid passwords" ;;
                esac

                # verify that the old password cannot be used anymore
                # and the new password can
                can_login "$TEST_USER" "$TEST_OLDPASS" && \
                    exit_fail "valid pw change failed: old password can be used"
                can_login "$TEST_USER" "$password" || \
                    exit_fail "valid pw change failed: new password cannot be used"

                # search audit log
                case "$2" in
                    passwd|su|login|ssh) ;;
                    *) exit_error "invalid try_auth type: $2" ;;
                esac
                ${2}_audit_pass "$auditmark" || \
                    exit_fail "$2: no audit entry for valid password"

                export TEST_OLDPASS="$password"
            done

            # invalid passwords
            for password in $PASSWD_INVALID; do
                passwd -e "$TEST_USER"
                faillock --user "$TEST_USER" --reset
                auditmark=$(get_audit_mark)

                if [ "$2" = "login" ]; then
                    "$0" "$1" "$2" "$password"
                else
                    run_user "$0" "$1" "$2" "$password"
                fi
                case "$?" in
                    0) exit_fail "user was able to specify password violating the rules" ;;
                    2) exit_error "error when testing interfaces for invalid passwords" ;;
                esac

                # we can't use can_login with old password for invalid passwords,
                # because we can't verify that the old password works, because
                # it doesn't, since it's already expired
                # - try new password only - it should fail on unexpected
                #   output (the password change prompt, "New password: ")
                can_login "$TEST_USER" "$password" && \
                    exit_fail "invalid pw change succeeded: new password can be used"

                case "$2" in
                    passwd|su|login|ssh) ;;
                    *) exit_error "invalid try_auth type: $2" ;;
                esac
                ${2}_audit_fail "$auditmark" || \
                    exit_fail "$2: no audit entry for invalid password"

                # don't set TEST_OLDPASS here, since the password is never meant
                # to be changed (for invalid passwords) - if it does, this test
                # fails anyway
            done

            ;;
        *)
            exit_error "unknown action for $0"
            ;;
    esac
    exit_pass
fi

#
# main, testing (mostly user), child part
#
# exit retval == 0: action succeeded
# exit retval == 1: action failed
# exit retval == 2: unexpected error occurred
#

case "$1" in
    'alter_rules')
        if [ "$2" = "user" ]; then
            # try to alter the password rules,
            # success in one of the cases is considered an overall success
            rules_pwquality_conf && exit 0
            rules_pamd_auth && exit 0
            exit 1
        else
            exit 2
        fi
        ;;
    'try_auth')
        # missing password?
        [ "$3" ] || exit 2

        # try auth and password change
        # - argument check
        case "$2" in
            passwd|su|login|ssh) ;;
            *) exit 2 ;;
        esac
        # - actual auth attempt
        ${2}_auth "$3"
        exit $?
        ;;
    *)
        exit 2
        ;;
esac

# vim: sts=4 sw=4 et :
