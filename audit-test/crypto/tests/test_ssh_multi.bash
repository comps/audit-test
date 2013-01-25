#!/bin/bash
###############################################################################
#   Copyright (c) 2011 Red Hat, Inc. All rights reserved.
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
# FIA_UAU.5
#
# AUTHOR: Miroslav Vadkerti <mvadkert@redhat.com>
#
# DESCRIPTION:
# This generates new RSA keys with a passphrase, tests if keys were
# correctly generated and uses them for passwordless connection via ssh
# between sysadm_u and user_u users. It then tries if fallback to password
# authentication works if bad passphrase given to pubkey authentication.
# For the last step it tries to use an incorrect password and tests if
# connection was denied.
#

source testcase.bash || exit 2
source tp_ssh_functions.bash || exit 2
source tp_selinux_functions.bash || exit 2

#### main ####

# global
PHRASE="anYP4ss"
MPROFILE="/etc/profile"
SSHDCONF="/etc/sysconfig/sshd"
CCCONF="/etc/profile.d/cc-configuration.sh"

# be verbose
set -x

# enable sysadm_u login via ssh
setsebool ssh_sysadm_login=1
append_cleanup "setsebool ssh_sysadm_login=0"

# backup global profile and remove sleep
backup $MPROFILE
backup $SSHDCONF
backup $CCCONF
ssh_remove_screen $MPROFILE

# remove SSH_USE_STRONG_RNG from environment
ssh_remove_strong_rng_env
# remove SSH_USE_STRONG_RNG from files
ssh_remove_strong_rng $SSHDCONF
ssh_remove_strong_rng $CCCONF
service sshd restart

# remove .ssh folders at cleanup
prepend_cleanup "ssh_cleanup_home $TEST_USER $TEST_USER_PASSWD"
prepend_cleanup "ssh_cleanup_home $TEST_ADMIN $TEST_ADMIN_PASSWD"

# clear faillock at cleanup
prepend_cleanup "faillock --reset --user $TEST_USER"
prepend_cleanup "faillock --reset --user $TEST_ADMIN"

original_params=("$@")
for USR in "$TEST_USER $TEST_USER_PASSWD" "$TEST_ADMIN $TEST_ADMIN_PASSWD"
do
    set -- $USR

    # create new rsa key with $PHRASE passphrase
    ssh_create_key $1 $2 rsa 1024 $PHRASE | grep "RSA 1024" || \
        exit_fail "Failed to create RSA $KEY key"

    # check if key created correctly
    ssh_check_home $1 $2 rsa
done
set -- $original_params

# copy RSA keys between the users
ssh_copy_key $TEST_ADMIN $TEST_ADMIN_PASSWD $TEST_USER $TEST_USER_PASSWD rsa
ssh_copy_key $TEST_USER $TEST_USER_PASSWD $TEST_ADMIN $TEST_ADMIN_PASSWD rsa

# test connection with rsa key
AUDITMARK=$(get_audit_mark)
ssh_connect_nopass $TEST_ADMIN $TEST_ADMIN_PASSWD $TEST_USER $PHRASE
ssh_check_audit $AUDITMARK
AUDITMARK=$(get_audit_mark)
ssh_connect_nopass $TEST_USER $TEST_USER_PASSWD $TEST_ADMIN $PHRASE
ssh_check_audit $AUDITMARK

# test connection with password and use bad rsa passphrase
# to skip pubkey auth
# syadm_u -> user_u
AUDITMARK=$(get_audit_mark)
ssh_connect_pass $TEST_ADMIN $TEST_ADMIN_PASSWD $TEST_USER \
    $TEST_USER_PASSWD || \
    exit_fail "Failed to connect from $TEST_ADMIN to $TEST_USER"
ssh_check_audit $AUDITMARK
# user_u -> sysadm_u
AUDITMARK=$(get_audit_mark)
ssh_connect_pass $TEST_USER $TEST_USER_PASSWD $TEST_ADMIN \
    $TEST_ADMIN_PASSWD || \
    exit_fail "Failed to connect from $TEST_USER to $TEST_ADMIN"
ssh_check_audit $AUDITMARK

# test connection with bad password and use bad rsa passphrase
# to skip pubkey auth
# syadm_u -> user_u
AUDITMARK=$(get_audit_mark)
ssh_connect_pass $TEST_ADMIN $TEST_ADMIN_PASSWD $TEST_USER \
    "BADPASS" && \
    exit_fail "Connected from $TEST_ADMIN to $TEST_USER with bad password"
ssh_check_audit $AUDITMARK fail
# clear faillock for $TEST_USER
faillock --reset --user $TEST_USER
# user_u -> sysadm_u
AUDITMARK=$(get_audit_mark)
ssh_connect_pass $TEST_USER $TEST_USER_PASSWD $TEST_ADMIN \
    "BADPASS" && \
    exit_fail "Connected from $TEST_USER to $TEST_ADMIN with bad password"
ssh_check_audit $AUDITMARK fail

exit_pass
