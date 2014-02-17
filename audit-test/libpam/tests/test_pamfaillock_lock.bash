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
# Verify pam_faillock will lock an account when logging in via ssh and console
# login
#
# SFRs: FIA_AFL.1
#

source pam_functions.bash || exit 2
source tp_ssh_functions.bash || exit 2

# make sure faillock is reset for TEST_USER before and after test
/sbin/faillock --user $TEST_USER --reset
prepend_cleanup "/sbin/faillock --user $TEST_USER --reset"

# setup
tuid=$(id -u $TEST_USER)

function try_ssh_login {
    [ -z "$1" ] && exit_error "No user name given for $FUNCNAME"
    [ -z "$2" ] && exit_error "No user password given for $FUNCNAME"
    [ -z "$3" ] && exit_error "No attempts count given for $FUNCNAME"

    expect -c "
        set max_attempts $3
        set attempts 0
        spawn ssh $1@localhost
        expect {
            {continue} { send \"yes\r\"; exp_continue }
            {assword} {
                if { \$attempts == \$max_attempts } { sleep 1; exit }
                send \"$2\r\"
                incr attempts
                exp_continue
            }
        }"
}

# ssh test
function test_ssh {
    local PAM_CONF=/etc/pam.d/password-auth
    backup $PAM_CONF

    grep -q pam_faillock $PAM_CONF || \
        exit_error "pam faillock not configured correctly in sshd"

    ### FIA_AFL.1 / Test 2
    # disable strong rng in the environment for the test
    # this will speed up things
    disable_ssh_strong_rng

    # the default of failed attempts is 3 in RHEL
    # see man pam_faillock
    AUDIT_MARK=$(get_audit_mark)
    try_ssh_login $TEST_USER badpass 3

    # check account locking logged in audit log
    msg_1="pam_faillock uid=$tuid.*exe=.$(which sshd).*res=success.*"
    augrok --seek=$AUDIT_MARK type=ANOM_LOGIN_FAILURES msg_1=~"$msg_1" || \
        exit_fail "Could not find ANOM_LOGIN_FAILURES audit message"
    augrok --seek=$AUDIT_MARK type=RESP_ACCT_LOCK msg_1=~"$msg_1" || \
        exit_fail "Could not find RESP_ACCT_LOCK audit message"

    # try to login to a failed account with correct password
    # this should fail
    expect -c "
        spawn ssh $TEST_USER@localhost
        expect {
            {continue} { send \"yes\r\"; exp_continue }
            {assword} { send \"$TEST_USER_PASSWD\r\" }
        }
        expect {
            {$TEST_USER} { exit 1 }
            {assword} { exit 0 }
            timeout { exit 2 }
        }"

    # check if could not login
    [ $? -eq 1 ] && exit_fail "Could login with locked account"
    [ $? -eq 2 ] && exit_error "Login with locked account failed unexpectedly"
    ### End FIA_AFL.1 / Test 2

    ### FIA_AFL.1 / Test 3
    /sbin/faillock --user $TEST_USER --reset

    # try to modify $PAM_CONF as $TEST_USER
    ssh_cmd $TEST_USER $TEST_USER_PASSWD "sed -i 's/.*pam_faillock.*//' $PAM_CONF" && \
        exit_fail "$TEST_USER could modify $PAM_CONF"
    ssh_cmd $TEST_USER $TEST_USER_PASSWD "echo > $PAM_CONF" && \
        exit_fail "$TEST_USER could modify $PAM_CONF"
    ### End FIA_AFL.1 / Test 3

    ### FIA_AFL.1 / Test 1
    /sbin/faillock --user $TEST_USER --reset

    # Set the failed attempts to 1
    sed -i 's/\(auth.*pam_faillock.so.*\)/\1 deny=1/' $PAM_CONF

    # try to login only once
    AUDIT_MARK=$(get_audit_mark)
    try_ssh_login $TEST_USER badpassword 1

    # check account locking logged in audit log
    msg_1="pam_faillock uid=$tuid.*exe=.$(which sshd).*res=success.*"
    augrok --seek=$AUDIT_MARK type=ANOM_LOGIN_FAILURES msg_1=~"$msg_1" || \
        exit_fail "Could not find ANOM_LOGIN_FAILURES audit msg (1 failure)"
    augrok --seek=$AUDIT_MARK type=RESP_ACCT_LOCK msg_1=~"$msg_1" || \
        exit_fail "Could not find RESP_ACCT_LOCK audit message (1 failure)"
    ### End FIA_AFL.1 / Test 1
}

function try_login {
    [ -z "$1" ] && exit_error "No user name given for $FUNCNAME"
    [ -z "$2" ] && exit_error "No user password given for $FUNCNAME"
    [ -z "$3" ] && exit_error "No attempts count given for $FUNCNAME"

    expect -c "
        set max_attempts $3
        set attempts 0
        spawn login
        sleep 1
        expect {
            {login:} { send \"$1\r\"; exp_continue }
            {assword:} {
                send \"$2\r\"
                incr attempts
                if { \$attempts == \$max_attempts } { sleep 1; exit }
                exp_continue
            }
        }
        "
}

function test_login {
    local PAM_CONF=/etc/pam.d/system-auth
    backup $PAM_CONF

    # In RHEL7 the pam_loginuid fails if loginuid already set
    # for the purpose of this test we disable it temporarily
    backup /etc/pam.d/login
    sed -i 's/\(^session.*pam_loginuid.*$\)/\#\1/' /etc/pam.d/login

    egrep -q "auth.*pam_faillock" $PAM_CONF || \
        exit_error "pam faillock not configured correctly in the system"

    ### FIA_AFL.1 / Test 2
    # check if account get's locked after 3 login attempts (the default)
    # see man pam_faillock
    # test incorrect login
    AUDIT_MARK=$(get_audit_mark)
    try_login $TEST_USER badpassword 3

    # check account locking logged in audit log
    msg_1="pam_faillock uid=$tuid.*exe=.$(which login).*res=success.*"
    augrok --seek $AUDIT_MARK type=ANOM_LOGIN_FAILURES msg_1=~"$msg_1" || \
        exit_fail "Could not find ANOM_LOGIN_FAILURES audit message"
    augrok --seek $AUDIT_MARK type=RESP_ACCT_LOCK msg_1=~"$msg_1" || \
        exit_fail "Could not find RESP_ACCT_LOCK audit message"

    # try to login to a failed account with correct password
    # this should fail
    expect -c "
        spawn login
        sleep 1
        expect {
            {login:} { send \"$TEST_USER\r\"; exp_continue }
            {assword:} {send \"$TEST_USER_PASSWD\r\"; exp_continue }
            {Login incorrect} { exit 0 }
        }
        exit 1"

    # check if could not login
    [ $? -eq 1 ] && exit_fail "Could login with locked account"
    [ $? -eq 2 ] && exit_error "Login with locked account failed unexpectedly"

    ### FIA_AFL.1 / Test 3
    /sbin/faillock --user $TEST_USER --reset

    # try to modify $PAM_CONF as $TEST_ADMIN
    local EXPOUT="$(expect -c "
        spawn login
        sleep 1
        expect {
            {login:} { send \"$TEST_ADMIN\r\"; exp_continue }
            {assword:} {send \"$TEST_ADMIN_PASSWD\r\"; exp_continue }
            {role or level} { send \r }
        }
        expect {$TEST_ADMIN} { send \"echo > $PAM_CONF; exit\r\" }
        expect eof")"
    echo $EXPOUT
    echo $EXPOUT | grep "$PAM_CONF: Permission denied" || \
        exit_fail "Permission not denied while writing to $PAM_CONF"
    ### End FIA_AFL.1 / Test 3

    ### FIA_AFL.1 / Test 1
    /sbin/faillock --user $TEST_USER --reset

    # Set the failed attempts to 1
    sed -i 's/\(auth.*pam_faillock.so.*\)/\1 deny=1/' $PAM_CONF

    # try to login only once
    AUDIT_MARK=$(get_audit_mark)
    try_login $TEST_USER badpassword 1

    # check account locking logged in audit log
    msg_1="pam_faillock uid=$tuid.*exe=.$(which login).*res=success.*"
    augrok --seek=$AUDIT_MARK type=ANOM_LOGIN_FAILURES msg_1=~"$msg_1" || \
        exit_fail "Could not find ANOM_LOGIN_FAILURES audit message"
    augrok --seek=$AUDIT_MARK type=RESP_ACCT_LOCK msg_1=~"$msg_1" || \
        exit_fail "Could not find RESP_ACCT_LOCK audit message"
}

# clean up
/sbin/faillock --user $TEST_USER --reset > /dev/null

# choose test
if [ "$(type -t test_$1)" = "function" ]; then
    eval test_$1
else
    exit_error "Uknown test or no test given"
fi

exit_pass
