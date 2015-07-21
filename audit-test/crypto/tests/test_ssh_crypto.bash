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
# The cipher test tries to connect with all required ciphers:
# 3des-cbc 3des-ctr aes192-cbc aes256-cbc aes128-ctr aes192-ctr aes256-ctr
# The test also checks if the sshd config is immutable to unauthorized access.
# Legal access of privileged user is tested via sshd_config_set function.
#
# The hmac test tests if all supported hmacs can be used for connecting
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

    # Try all cipher types
    for CIPHER in $CIPHERS; do
        AUDITMARK=$(get_audit_mark)
        ssh_connect_pass $TEST_USER $TEST_USER_PASSWD \
            $TEST_ADMIN $TEST_ADMIN_PASSWD "-c $CIPHER" || \
            exit_fail "Failed to connect from $TEST_USER to $TEST_ADMIN"
        ssh_check_audit $AUDITMARK pass $CIPHER

        # test in simulated FIPS mode
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

    # Try incompatible cipher types
    sshd_config_set "Ciphers" "aes256-ctr"
    AUDITMARK=$(get_audit_mark)
    ssh_connect_pass $TEST_USER $TEST_USER_PASSWD \
        $TEST_ADMIN $TEST_ADMIN_PASSWD "-c 3des-cbc"; local RET=$?
    [ $RET -eq 0 ] && exit_fail \
        "Connection successful with incompatible ciphers"
    [ $RET -ne 6 ] && exit_error "Unexpected error message"

    # Try invalid cipher type with all supported ciphers, only in non-FIPS mode
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

    # FMT_MTD.1(CM) - Test 3
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

        AUDITMARK=$(get_audit_mark)
        ssh_connect_pass $TEST_USER $TEST_USER_PASSWD \
            $TEST_ADMIN $TEST_ADMIN_PASSWD "-m $HMAC" || \
            exit_fail "Failed to connect from $TEST_USER to $TEST_ADMIN"
        ssh_check_audit $AUDITMARK pass

        # try out all ciphers
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

# Try out connecting with SSH_USE_STRONG_RNG enabled in env
# aes128-cbc CIPHER should be enabled by default
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

# run testing
hmac

exit_pass
