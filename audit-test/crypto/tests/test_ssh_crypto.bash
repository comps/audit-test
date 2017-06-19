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
# SFRs: FCS_COP.1(NET) FMT_MTD.1(CM)
#
# AUTHOR: Miroslav Vadkerti <mvadkert@redhat.com>
#
# DESCRIPTION:
# This test implements multiple test cases. Tests 3., 4. and 6. iterate
# through all HMAC x CIPHER combinations.
#
# Test 1. - verify successful login from an ordinary (user_u) and admin user
#           (sysadm_u) and vice versa with enabled strong rng and a supported
#           cipher
#
# Test 2. - verify that successful login with a specific HMAC works correctly
#
# Test 3. - try out all HMAC x CIPHER combinations and verify that login
#           from ordinary to admin user works and it is audited correctly
#
# Test 4. - try out login with all HMAC x CIPHER combinations
#           in simulated FIPS mode and verify that it works for
#           FIPS supported HMACs and does not work for not supported
#           HMACs
#
# Test 5. - for all HMACs verify that login with and incompatible cipher type
#           does not work
#
# Test 6. - for all HMAC x CIPHER combinations verify that login with an invalid
#           cipher does not work
#

source testcase.bash || exit 2
source tp_ssh_functions.bash || exit 2

#### main ####

# globals
MPROFILE="/etc/profile"
SSHDCONF="/etc/sysconfig/sshd"
CCCONF="/etc/profile.d/cc-configuration.sh"
CIPHERS="3des-cbc aes128-cbc aes192-cbc aes256-cbc aes128-ctr \
aes192-ctr aes256-ctr aes128-gcm@openssh.com aes256-gcm@openssh.com"
HMACS="hmac-md5 hmac-md5-96 hmac-sha1 hmac-sha1-96 hmac-sha2-256 \
hmac-sha2-512 hmac-md5-etm@openssh.com hmac-sha1-etm@openssh.com \
hmac-sha2-256-etm@openssh.com hmac-sha2-512-etm@openssh.com \
hmac-sha1-96-etm@openssh.com"
HMACS_FIPS="hmac-sha1 hmac-sha2-256 hmac-sha2-512 hmac-sha1-etm@openssh.com \
hmac-sha2-256-etm@openssh.com hmac-sha2-512-etm@openssh.com"

# be verbose
set -x

# enable sysadm_u login via ssh
setsebool ssh_sysadm_login=1
append_cleanup "setsebool ssh_sysadm_login=0"

# install dracut-fips if needed to have a valid FIPS product
# when testing with OPENSSL_FORCE_FIPS_MODE env variable
if ! rpm -q dracut-fips; then
    touch /etc/system-fips
    append_cleanup "rm -f /etc/system-fips"
fi

function cipher {
    # Set all ciphers as supported
    sshd_config_set "Ciphers" "$(echo "$CIPHERS" | sed 's/ /,/g')"

    for CIPHER in $CIPHERS; do
        # Test 3. - try out successful login with all HMAC x CIPHER combinations
        AUDITMARK=$(get_audit_mark)
        ssh_connect_pass $TEST_USER $TEST_USER_PASSWD \
            $TEST_ADMIN $TEST_ADMIN_PASSWD "-c $CIPHER" || \
            exit_fail "Failed to connect from $TEST_USER to $TEST_ADMIN"
        ssh_check_audit $AUDITMARK pass $CIPHER

        # Test 4. - try out login with all HMAC x CIPHER combinations
        #           in simulated FIPS mode and verify that it works for
        #           FIPS supported HMACs and does not work for not supported
        #           HMACs
        AUDITMARK=$(get_audit_mark)
        if grep -q "$HMAC" <<< "$HMACS_FIPS"; then
            OPENSSL_FORCE_FIPS_MODE=1 ssh_connect_pass $TEST_USER $TEST_USER_PASSWD \
                $TEST_ADMIN $TEST_ADMIN_PASSWD "-c $CIPHER" || \
                exit_fail "Failed to connect from $TEST_USER to $TEST_ADMIN"
            ssh_check_audit $AUDITMARK pass $CIPHER
        else
            OPENSSL_FORCE_FIPS_MODE=1 ssh_connect_pass $TEST_USER $TEST_USER_PASSWD \
                $TEST_ADMIN $TEST_ADMIN_PASSWD "-c $CIPHER" && \
                exit_fail "Succeeded to connect with invalid MAC in FIPS"
        fi
    done

    # Test 5. - verify that login with incompatible cipher type does not work
    sshd_config_set "Ciphers" "aes256-ctr"
    AUDITMARK=$(get_audit_mark)
    ssh_connect_pass $TEST_USER $TEST_USER_PASSWD \
        $TEST_ADMIN $TEST_ADMIN_PASSWD "-c 3des-cbc"; local RET=$?
    [ $RET -eq 0 ] && exit_fail \
        "Connection successful with incompatible ciphers"
    [ $RET -ne 6 ] && exit_error "Unexpected error message"

    # Test 6. - verify that login with an invalid cipher type does not work
    if ! is_fips; then
        sshd_config_set "Ciphers" "blowfish-cbc"
        # Try all cipher types
        for CIPHER in $CIPHERS; do
            AUDITMARK=$(get_audit_mark)
            ssh_connect_pass $TEST_USER $TEST_USER_PASSWD \
                $TEST_ADMIN $TEST_ADMIN_PASSWD "-c $CIPHER" && \
                exit_fail "Connection successful with invalid cipher ($CIPHER)"
        done
    fi

    # FMT_MTD.1(CM)
    # check if admin user is unable to modify $SSHDCONF
    ssh_cmd $TEST_ADMIN $TEST_ADMIN_PASSWD "echo SOMETHING >> $SSHDCONF" && \
        exit_fail "Modifying $SSHDCONF was not denied"
}


function hmac {
    # Try out all HMAC types (in FIPS mode limit to HMACS_FIPS)
    is_fips && HMACS="$HMACS_FIPS"
    for HMAC in $HMACS; do
        # make sure server supports only one MAC
        sshd_config_set "MACs" "$HMAC"

        # Test 2. - verify that a successful login with only a specific
        #           enabled HMAC works
        AUDITMARK=$(get_audit_mark)
        ssh_connect_pass $TEST_USER $TEST_USER_PASSWD \
            $TEST_ADMIN $TEST_ADMIN_PASSWD "-m $HMAC" || \
            exit_fail "Failed to connect from $TEST_USER to $TEST_ADMIN"
        ssh_check_audit $AUDITMARK pass

        # try out all ciphers with one enabled HMAC
        cipher
    done

    # Try out incompatible HMACs
    sshd_config_set "MACs" "hmac-sha1"
    AUDITMARK=$(get_audit_mark)
    ssh_connect_pass $TEST_USER $TEST_USER_PASSWD \
        $TEST_ADMIN $TEST_ADMIN_PASSWD "-m hmac-sha2-256-etm@openssh.com"; local RET=$?
    [ $RET -eq 0 ] && exit_fail \
        "Connection successful with incompatible hmacs"
    [ $RET -ne 5 ] && exit_error "Unexpected connection failure"
}

# Test 1. - try out connecting with SSH_USE_STRONG_RNG enabled in env
for CIPHER in aes128-cbc; do
    AUDITMARK=$(get_audit_mark)
    ssh_connect_pass $TEST_ADMIN $TEST_ADMIN_PASSWD \
        $TEST_USER $TEST_USER_PASSWD "-c $CIPHER" || \
        exit_fail "Failed to connect from $TEST_ADMIN to $TEST_USER"
    ssh_check_audit $AUDITMARK pass $CIPHER
    AUDITMARK=$(get_audit_mark)
    ssh_connect_pass $TEST_USER $TEST_USER_PASSWD \
        $TEST_ADMIN $TEST_ADMIN_PASSWD "-c $CIPHER" || \
        exit_fail "Failed to connect from $TEST_USER to $TEST_ADMIN"
    ssh_check_audit $AUDITMARK pass $CIPHER
done

# remove SSH_USE_STRONG_RNG from environment and disable screen
disable_ssh_strong_rng

hmac

exit_pass
