#!/bin/bash
###############################################################################
#   Copyright (c) 2011, 2014 Red Hat, Inc. All rights reserved.
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
# FIA_UAU.1(RITE), FIA_UAU.5
#
# AUTHOR: Miroslav Vadkerti <mvadkert@redhat.com>
#
# DESCRIPTION:
# This file incorporates tests for FIA_UAU.1(RITE), FIA_UAU.5 coverage.
# See the description below for specific subtests and their purpose.
#
# ssh_multi login
#   This generates new ECDSA keys with a passphrase, tests if keys were
#   correctly generated and uses them for passwordless connection via ssh
#   between sysadm_u and user_u users. It tests if login works exclusively
#   with pubkey authentication only (i.e. fails if no valid pubkey available,
#   or wrong pubkey password supplied). In the second part of the test case
#   the pubkey authentication method is disabled and only success/pass
#   of password based autentication is tested.
#
# ssh_multi fallback
#   This generates new RSA keys with a passphrase, tests if keys were
#   correctly generated and uses them for passwordless connection via ssh
#   between sysadm_u and user_u users. It then tries if fallback to password
#   authentication works if bad passphrase given to pubkey authentication.
#   For the last step it tries to use an incorrect password and tests if
#   connection was denied.
#
# ssh_multi account_expire
#   Checks if user with expired account (chage -E 0 user) is not able to login
#   with valid password or pubkey.
#
# ssh_multi password_expire
#   Checks if user with expired password (chage -d -1 user & chage -M 0 user)
#   is not able to login with valid password or pubkey.
#

source testcase.bash || exit 2
source tp_ssh_functions.bash || exit 2

# global
PHRASE="anYP4ss32"

# be verbose
set -x

###############################################################################
## Setup phase
###############################################################################

function ssh_multi_setup {
    # enable sysadm_u login via ssh
    setsebool ssh_sysadm_login=1
    append_cleanup "setsebool ssh_sysadm_login=0"

    # backup global profile and remove sleep
    disable_ssh_strong_rng

    # remove .ssh folders at cleanup
    prepend_cleanup "ssh_cleanup_home $TEST_USER $TEST_USER_PASSWD"
    prepend_cleanup "ssh_cleanup_home $TEST_ADMIN $TEST_ADMIN_PASSWD"

    # clear faillock at cleanup
    prepend_cleanup "faillock --reset --user $TEST_USER"
    prepend_cleanup "faillock --reset --user $TEST_ADMIN"
}

###############################################################################
## FIA_UAU.1(RITE)
###############################################################################

function login {
    # setup
    ssh_multi_setup

    # backup root .ssh
    ssh_cleanup_home -r ecdsa

    # create new ecdsa key for running user and distribute it to test user
    # for passwordless login
    ssh_create_key -r ecdsa 384 | egrep "ECDSA[[:space:]]+384" || \
        exit_fail "Failed to create ECDSA key"
    ssh_copy_key -r $TEST_USER $TEST_USER_PASSWD ecdsa

    # create new ecdsa key for test user with passphrase and check if correctly created
    # the copy the key to test admin
    ssh_create_key $TEST_USER $TEST_USER_PASSWD ecdsa 384 $PHRASE | egrep "ECDSA[[:space:]]+384" || \
        exit_fail "Failed to create ECDSA key"
    ssh_check_home $TEST_USER $TEST_USER_PASSWD ecdsa
    ssh_copy_key $TEST_USER $TEST_USER_PASSWD $TEST_ADMIN $TEST_ADMIN_PASSWD ecdsa

    # enable only pubkey login
    sshd_config_set "ChallengeResponseAuthentication" "no"

    # positive test - pubkey auth with correct passphrase
    ssh_connect_key $TEST_USER $TEST_USER_PASSWD $TEST_ADMIN $PHRASE || \
        exit_fail "Failed to connect from $TEST_USER to $TEST_ADMIN"

    # negative test - pubkey auth with invalid passphrase
    ssh_connect_key $TEST_USER $TEST_USER_PASSWD $TEST_ADMIN BADPASS; local RET=$?
    [ $RET -eq 0 ] && exit_fail "Connection passed with invalid passphrase"
    [ $RET -eq 5 ] || exit_fail "Connection failed unexpectedly ($RET)"

    # negative test - no key available
    ssh_cleanup_home $TEST_USER $TEST_USER_PASSWD ecdsa
    ssh_connect_key $TEST_USER $TEST_USER_PASSWD $TEST_ADMIN $PHRASE; local RET=$?
    [ $RET -eq 0 ] && exit_fail "Connection passed with invalid passphrase"
    [ $RET -eq 5 ] || exit_fail "Connection failed unexpectedly ($RET)"

    # negative test - invalid pubkey
    ssh_cleanup_home $TEST_USER $TEST_USER_PASSWD ecdsa
    ssh_create_key $TEST_USER $TEST_USER_PASSWD ecdsa 384 OTHERPASS | egrep "ECDSA[[:space:]]+384" || \
        exit_fail "Failed to create ECDSA key"
    ssh_connect_key $TEST_USER $TEST_USER_PASSWD $TEST_ADMIN BADPASS; local RET=$?
    [ $RET -eq 0 ] && exit_fail "Connection passed with invalid passphrase"
    [ $RET -eq 5 ] || exit_fail "Connection failed unexpectedly ($RET)"

    # enable only password login
    sshd_config_set "PubkeyAuthentication" "no"
    sshd_config_set "ChallengeResponseAuthentication" "yes"

    # positive test - passwd auth with correct password
    ssh_connect_pass $TEST_USER $TEST_USER_PASSWD $TEST_ADMIN $TEST_ADMIN_PASSWD || \
        exit_fail "Failed to connect from $TEST_USER to $TEST_ADMIN with correct password"

    # negative test - passwd auth with invalid password
    ssh_connect_pass $TEST_USER $TEST_USER_PASSWD $TEST_ADMIN BADPASS; local RET=$?
    [ $RET -eq 0 ] && exit_fail "Connection passed with invalid password"
    [ $RET -eq 2 ] || exit_fail "Connection failed unexpectedly ($RET)"
}

###############################################################################
## FIA_UAU.5
###############################################################################

function fallback {
    # setup
    ssh_multi_setup

    for USR in TEST_USER TEST_ADMIN
    do
        USRNAME=$(eval echo \$${USR})
        USRPASS=$(eval echo \$${USR}_PASSWD)

        # create new rsa key with $PHRASE passphrase
        ssh_create_key $USRNAME $USRPASS rsa 1024 $PHRASE | grep "RSA 1024" || \
            exit_fail "Failed to create RSA $KEY key"

        # check if key created correctly
        ssh_check_home $USRNAME $USRPASS rsa
    done

    # copy RSA keys between the users
    ssh_copy_key $TEST_ADMIN $TEST_ADMIN_PASSWD $TEST_USER $TEST_USER_PASSWD rsa
    ssh_copy_key $TEST_USER $TEST_USER_PASSWD $TEST_ADMIN $TEST_ADMIN_PASSWD rsa

    # test connection with rsa key
    AUDITMARK=$(get_audit_mark)
    ssh_connect_key $TEST_ADMIN $TEST_ADMIN_PASSWD $TEST_USER $PHRASE || \
        exit_fail "Failed to connect from $TEST_ADMIN to $TEST_USER"
    ssh_check_audit $AUDITMARK
    AUDITMARK=$(get_audit_mark)
    ssh_connect_key $TEST_USER $TEST_USER_PASSWD $TEST_ADMIN $PHRASE || \
        exit_fail "Failed to connect from $TEST_ADMIN to $TEST_USER"
    ssh_check_audit $AUDITMARK

    # test connection with password and use bad rsa passphrase
    # to skip pubkey auth
    # in MLS sysadm_u -> user_u
    AUDITMARK=$(get_audit_mark)
    ssh_connect_pass $TEST_ADMIN $TEST_ADMIN_PASSWD $TEST_USER \
        $TEST_USER_PASSWD || \
        exit_fail "Failed to connect from $TEST_ADMIN to $TEST_USER"
    ssh_check_audit $AUDITMARK
    # in MLS user_u -> sysadm_u
    AUDITMARK=$(get_audit_mark)
    ssh_connect_pass $TEST_USER $TEST_USER_PASSWD $TEST_ADMIN \
        $TEST_ADMIN_PASSWD || \
        exit_fail "Failed to connect from $TEST_USER to $TEST_ADMIN"
    ssh_check_audit $AUDITMARK

    # test connection with bad password and use bad rsa passphrase
    # to skip pubkey auth
    # in MLS sysadm_u -> user_u
    AUDITMARK=$(get_audit_mark)
    ssh_connect_pass $TEST_ADMIN $TEST_ADMIN_PASSWD $TEST_USER \
        "BADPASS" && \
        exit_fail "Connected from $TEST_ADMIN to $TEST_USER with bad password"
    ssh_check_audit $AUDITMARK fail
    # clear faillock for $TEST_USER
    faillock --reset --user $TEST_USER
    # in MLS user_u -> sysadm_u
    AUDITMARK=$(get_audit_mark)
    ssh_connect_pass $TEST_USER $TEST_USER_PASSWD $TEST_ADMIN \
        "BADPASS" && \
        exit_fail "Connected from $TEST_USER to $TEST_ADMIN with bad password"
    ssh_check_audit $AUDITMARK failA
}

function password_expire {
    # setup
    ssh_multi_setup

    # create new ecdsa key for test user with passphrase and check if correctly created
    # the copy the key to test admin
    ssh_create_key $TEST_USER $TEST_USER_PASSWD ecdsa 384 $PHRASE | egrep "ECDSA[[:space:]]+384" || \
        exit_fail "Failed to create ECDSA key"
    ssh_check_home $TEST_USER $TEST_USER_PASSWD ecdsa
    ssh_copy_key $TEST_USER $TEST_USER_PASSWD $TEST_ADMIN $TEST_ADMIN_PASSWD ecdsa

    # regular login
    # in MLS user_u->sysadm_u
    AUDITMARK=$(get_audit_mark)
    ssh_connect_key $TEST_USER $TEST_USER_PASSWD $TEST_ADMIN $PHRASE  || \
        exit_fail "Failed to connect from $TEST_USER to $TEST_ADMIN"
    ssh_check_audit $AUDITMARK

    # expired password - with password login
    # in MLS user_u->sysadm_u
    AUDITMARK=$(get_audit_mark)
    expire_account $TEST_ADMIN
    ssh_connect_key $TEST_USER $TEST_USER_PASSWD $TEST_ADMIN $PHRASE; local RET=$?
    [ $RET -eq 0 ] && exit_fail "Succeeded to connect with expired password from $TEST_USER to $TEST_ADMIN"
    [ $RET -eq 6 ] || exit_error "Connection failed unexpectedly. No expire info?"

    ssh_connect_pass $TEST_USER $TEST_USER_PASSWD $TEST_ADMIN $TEST_ADMIN_PASSWD; local RET=$?
    [ $RET -eq 0 ] && exit_fail "Succeeded to connect with expired password from $TEST_USER to $TEST_ADMIN"
    [ $RET -eq 4 ] || exit_error "Connection failed unexpectedly. No expire info?"
}

# choose test
if [ "$(type -t $1)" = "function" ]; then
    eval $1
else
    exit_error "Uknown test or no test given"
fi

exit_pass
