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
# SFR: FCS_COP.1(NET)
#
# AUTHOR: Miroslav Vadkerti <mvadkert@redhat.com>
#
# DESCRIPTION:
# Tries to connect with all required ciphers and HMAC-SHA1 mac:
# + TDES in CBC mode with 168 bits key size and HMAC-SHA1
# + AES in CBC mode with 128 bits and 256 bits key size and HMAC-SHA1
# + AES in CBC mode with 192 bits key size and HMAC-SHA1
# + AES in CTR mode with 128 bits, 192 bits and 256 bits key size
#   and HMAC-SHA1
# Also tests if audit log logs required records with correct ciphers
#

source testcase.bash || exit 2
source tp_ssh_functions.bash || exit 2
source tp_selinux_functions.bash || exit 2

#### main ####

# globals
MPROFILE="/etc/profile"
SSHDCONF="/etc/sysconfig/sshd"
CCCONF="/etc/profile.d/cc-configuration.sh"

# be verbose
set -x

# backup global profile and remove sleep
backup $MPROFILE
backup $SSHDCONF
backup $CCCONF
ssh_remove_screen $MPROFILE

# enable sysadm_u login via ssh
setsebool ssh_sysadm_login=1
append_cleanup "setsebool ssh_sysadm_login=0"

# Try few key sizes with USE_SSH_CRYPTO_RNG enabled
for CRYPTO in aes128-cbc; do
    # test connection from admin->user and vice versa
    AUDITMARK=$(get_audit_mark)
    ssh_connect_pass $TEST_ADMIN $TEST_ADMIN_PASSWD \
        $TEST_USER $TEST_USER_PASSWD "-c $CRYPTO -m hmac-sha1" || \
        exit_fail "Failed to connect from $TEST_ADMIN to $TEST_USER"
    ssh_check_audit $AUDITMARK pass $CRYPTO
    AUDITMARK=$(get_audit_mark)
    ssh_connect_pass $TEST_USER $TEST_USER_PASSWD \
        $TEST_ADMIN $TEST_ADMIN_PASSWD "-c $CRYPTO -m hmac-sha1" || \
        exit_fail "Failed to connect from $TEST_USER to $TEST_ADMIN"
    ssh_check_audit $AUDITMARK pass $CRYPTO
done

# remove SSH_USE_STRONG_RNG from environment
ssh_remove_strong_rng_env
# remove SSH_USE_STRONG_RNG from files
ssh_remove_strong_rng $SSHDCONF
ssh_remove_strong_rng $CCCONF
service sshd restart

# Try all key sizes
for CRYPTO in 3des-cbc aes192-cbc aes256-cbc aes128-ctr \
    aes192-ctr aes256-ctr; do
    # test connection from admin->user and vice versa
    AUDITMARK=$(get_audit_mark)
    ssh_connect_pass $TEST_ADMIN $TEST_ADMIN_PASSWD \
        $TEST_USER $TEST_USER_PASSWD "-c $CRYPTO -m hmac-sha1" || \
        exit_fail "Failed to connect from $TEST_ADMIN to $TEST_USER"
    ssh_check_audit $AUDITMARK pass $CRYPTO
    AUDITMARK=$(get_audit_mark)
    ssh_connect_pass $TEST_USER $TEST_USER_PASSWD \
        $TEST_ADMIN $TEST_ADMIN_PASSWD "-c $CRYPTO -m hmac-sha1" || \
        exit_fail "Failed to connect from $TEST_USER to $TEST_ADMIN"
    ssh_check_audit $AUDITMARK pass $CRYPTO
done

exit_pass
