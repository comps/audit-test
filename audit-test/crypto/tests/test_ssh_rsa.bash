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
# FCS_CKM.1(RSA), FCS_CKM.2(NET)
#
# AUTHOR: Miroslav Vadkerti <mvadkert@redhat.com>
#
# DESCRIPTION:
# This test generates RSA keys with key sizes 1024, 2048 and 3072 bits.
# Checks if keys are correctly generated in all cases and uses them
# for password-less connection between user_u and sysadm_u users.
# Also checks for # correct audit logs logged by ssh daemon.
#
# IMPORTANT:
# During the test the sleep command from /etc/profile is removed and
# it is restored after the test.
#

source testcase.bash || exit 2
source tp_ssh_functions.bash || exit 2
source tp_selinux_functions.bash || exit 2

#### main ####

# globals
KEY_SIZE="1024 2048 3072"
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

# cleanup .ssh folders
prepend_cleanup "ssh_cleanup_home $TEST_USER $TEST_USER_PASSWD"
prepend_cleanup "ssh_cleanup_home $TEST_ADMIN $TEST_ADMIN_PASSWD"

# Try all key sizes
for KEY in $KEY_SIZE; do
    # cleanup .ssh folders before an interation
    ssh_cleanup_home $TEST_USER $TEST_USER_PASSWD
    ssh_cleanup_home $TEST_ADMIN $TEST_ADMIN_PASSWD

    original_params=("$@")
    for USR in "$TEST_USER $TEST_USER_PASSWD" "$TEST_ADMIN $TEST_ADMIN_PASSWD"
    do
        set -- $USR

        # create new rsa key
        ssh_create_key $1 $2 rsa $KEY | grep "RSA $KEY" || \
            exit_fail "Failed to create RSA $KEY key"

        # check if key created with correct length
        ssh_check_key $1 $2 rsa $KEY

        # check if key created with correct permissions and SELinux context
        ssh_check_home $1 $2 rsa $KEY
    done
    set -- $original_params

    # copy RSA keys between the users
    ssh_copy_key $TEST_ADMIN $TEST_ADMIN_PASSWD $TEST_USER $TEST_USER_PASSWD rsa
    ssh_copy_key $TEST_USER $TEST_USER_PASSWD $TEST_ADMIN $TEST_ADMIN_PASSWD rsa

    # test connection without password
    AUDITMARK=$(get_audit_mark)
    ssh_connect_nopass $TEST_ADMIN $TEST_ADMIN_PASSWD $TEST_USER
    ssh_check_audit $AUDITMARK
    AUDITMARK=$(get_audit_mark)
    ssh_connect_nopass $TEST_USER $TEST_USER_PASSWD $TEST_ADMIN
    ssh_check_audit $AUDITMARK
done

exit_pass
