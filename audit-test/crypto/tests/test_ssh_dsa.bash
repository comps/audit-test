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
# FCS_CKM.1(DSA), FCS_CKM.2(NET)
#
# AUTHOR: Miroslav Vadkerti <mvadkert@redhat.com>
#
# DESCRIPTION:
# This test generates new DSA keys, checks if keys are correctly generated
# and uses them for password-less connection via ssh. Also checks for
# correct audit logs logged by ssh daemon.
#

source testcase.bash || exit 2
source tp_ssh_functions.bash || exit 2

#### main ####

# globals
MPROFILE="/etc/profile"
SSHDCONF="/etc/sysconfig/sshd"
CCCONF="/etc/profile.d/cc-configuration.sh"

# be verbose
set -x

# enable sysadm_u login via ssh
setsebool ssh_sysadm_login=1
append_cleanup "setsebool ssh_sysadm_login=0"

# backup global profile and remove sleep and screen
backup $MPROFILE
backup $SSHDCONF
backup $CCCONF
ssh_remove_screen $MPROFILE

# remove SSH_USE_STRONG_RNG from environment
ssh_remove_strong_rng_env
# remove SSH_USE_STRONG_RNG from files
ssh_remove_strong_rng $SSHDCONF
ssh_remove_strong_rng $CCCONF
restart_service sshd

# run for both test users
original_params=("$@")
for USR in "$TEST_USER $TEST_USER_PASSWD" "$TEST_ADMIN $TEST_ADMIN_PASSWD"
do
    set -- $USR

    # .ssh cleanup
    append_cleanup "ssh_cleanup_home $1 $2"

    # create new dsa key
    ssh_create_key $1 $2 dsa 1024 | grep "DSA 1024" || \
        exit_fail "Failed to create DSA 1024 key"

    # check if key created with correct length
    ssh_check_key $1 $2 dsa 1024

    # check if key created correctly only for $TEST_USER
    ssh_check_home $1 $2 dsa
done
set -- $original_params

# copy DSA keys between the users
ssh_copy_key $TEST_ADMIN $TEST_ADMIN_PASSWD $TEST_USER $TEST_USER_PASSWD dsa
ssh_copy_key $TEST_USER $TEST_USER_PASSWD $TEST_ADMIN $TEST_ADMIN_PASSWD dsa

# test connection without password with DSA keys between users
AUDITMARK=$(get_audit_mark)
ssh_connect_nopass $TEST_ADMIN $TEST_ADMIN_PASSWD $TEST_USER
ssh_check_audit $AUDITMARK
AUDITMARK=$(get_audit_mark)
ssh_connect_nopass $TEST_USER $TEST_USER_PASSWD $TEST_ADMIN
ssh_check_audit $AUDITMARK

exit_pass
