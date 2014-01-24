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
# Reference SFRs:
#  FCS_CKM.1(ECDSA) FCS_CKM.1(DSA) FCS_CKM.1(RSA) FCS_CKM.2(NET) FCS_COP.1(NET)
#  FMT_MTD.1(CM)
#
# AUTHOR:
#  Miroslav Vadkerti <mvadkert@redhat.com>
#
# DESCRIPTION:
#  This test tests user and host SSH keys according to the passed argument.
#
#  If the first parameter is rsa, dsa or ecdsa - this test generates SSH
#  user keys with specific key sizes according to the SFRs and then uses them
#  for passwordless login between TEST_USER and TEST_ADMIN. The correctness
#  of the generated keys is verified using the openssl tool. The test also
#  checks correct permissions and SELinux context of the generated keys.
#
#  If the first passed parameter is host - host keys test is executed. This
#  generates host keys in all supported types and key sizes and checkes if
#  they work well if strict host key checking required.
#
# IMPORTANT:
#  During the test the sleep command from /etc/profile is removed and
#  it is restored after the test.
#

source testcase.bash || exit 2
source tp_ssh_functions.bash || exit 2

#### main ####

# globals
KEY_TYPE=$1
ECDSA_KEY_SIZE="256 384 521"
DSA_KEY_SIZE="1024"
RSA_KEY_SIZE="1024 2048 3072 4096"
MPROFILE="/etc/profile"
SSHDCONF="/etc/sysconfig/sshd"
CCCONF="/etc/profile.d/cc-configuration.sh"

# be verbose
set -x

# remove SSH_USE_STRONG_RNG from environment and disable screen
# during the test
disable_ssh_strong_rng

# enable sysadm_u login via ssh
setsebool ssh_sysadm_login=1
append_cleanup "setsebool ssh_sysadm_login=0"

# host keys test
function test_host {
    # backup the sshd host keys and remove them
    backup /etc/ssh/ssh_host_*_key /etc/ssh/ssh_host_*_key.pub /root/.ssh/known_hosts
    rm -f /etc/ssh/ssh_host_*_key /etc/ssh/ssh_host_*_key.pub /root/.ssh/known_hosts

    # without host keys we should not be able to connect
    ssh_connect_pass $TEST_USER $TEST_USER_PASSWD $TEST_ADMIN $TEST_ADMIN_PASSWD &&
        exit_fail "Connection passed without host keys"

    # generate host keys with all key sizes and try to connect via scricthost checking
    local KEY_TYPES="dsa rsa ecdsa"
    for KEY_TYPE in $KEY_TYPES; do
        local KEY_TYPE_CAP="${KEY_TYPE^^}"
        for KEY_SIZE in $(eval echo \$${KEY_TYPE_CAP}_KEY_SIZE); do
            # cleanup all host keys
            rm -f /etc/ssh/ssh_host_*_key /etc/ssh/ssh_host_*_key.pub
            rm -f /root/.ssh/known_hosts

            # generate host key with given key size
            ssh-keygen -q -t $KEY_TYPE -b $KEY_SIZE -f \
                /etc/ssh/ssh_host_${KEY_TYPE}_key -C '' -N ''
            restorecon -RvvF /etc/ssh/ssh_host_${KEY_TYPE}_key{,.pub}
            restart_service sshd

            # clean test user ssh directory
            ssh_cleanup_home $TEST_USER $TEST_USER_PASSWD

            # try to connect with strict host key checking to uknown host
            ssh_connect_pass $TEST_USER $TEST_USER_PASSWD $TEST_ADMIN $TEST_ADMIN_PASSWD \
                "-o StrictHostKeyChecking=yes -o PasswordAuthentication=no"; local RET=$?
            [ $RET -eq 0 ] && exit_fail "Connection passed to uknown host"
            [ $RET -ne 7 ] && exit_error "Connection failed with wrong error"

            # copy the key to known hosts
            ssh_cmd $TEST_USER $TEST_USER_PASSWD \
                "echo 'localhost $(cat /etc/ssh/ssh_host_${KEY_TYPE}_key.pub)' >> ~/.ssh/known_hosts"
            ssh_cmd $TEST_USER $TEST_USER_PASSWD "cat ~/.ssh/known_hosts"

            # try to connect with strict host key checking
            ssh_connect_pass $TEST_USER $TEST_USER_PASSWD $TEST_ADMIN $TEST_ADMIN_PASSWD \
                "-o StrictHostKeyChecking=yes -o PasswordAuthentication=no" || \
		    exit_fail "Connection failed with valid host key"
        done
    done

    exit_pass
}

# execute test function if exists
if [ "$(type -t test_$1)" = "function" ]; then
    eval test_$1
fi

# sanity check for user keys tests
[[ "$KEY_TYPE" =~ (ecdsa|dsa|rsa) ]] || \
    exit_error "Unsupported or none key type specified. " \
        "Must be one of ecdsa, rsa or dsa."
KEY_TYPE_CAP="${KEY_TYPE^^}"

# cleanup .ssh folders
prepend_cleanup "ssh_cleanup_home $TEST_USER $TEST_USER_PASSWD"
prepend_cleanup "ssh_cleanup_home $TEST_ADMIN $TEST_ADMIN_PASSWD"

# Try all key sizes in give key type
# FCS_CKM.1(ECDSA) FCS_CKM.1(DSA), FCS_CKM.1(RSA)
for KEY in $(eval echo \$${KEY_TYPE_CAP}_KEY_SIZE); do

    # create public keys of KEY type for TUSER and TADMIN
    for USR in TEST_USER TEST_ADMIN
    do
        USRNAME=$(eval echo \$${USR})
        USRPASS=$(eval echo \$${USR}_PASSWD)

        # cleanup .ssh folders before an interation
        ssh_cleanup_home $USRNAME $USRPASS
        ssh_cleanup_home $USRNAME $USRPASS

        # create new $KEY_TYPE key
        ssh_create_key $USRNAME $USRPASS $KEY_TYPE $KEY | egrep "$KEY_TYPE_CAP[[:space:]]+$KEY" || \
            exit_fail "Failed to create $KEY_TYPE $KEY key"

        # check if key created with correct length
        ssh_check_key $USRNAME $USRPASS $KEY_TYPE $KEY

    done

    # copy keys between the users
    ssh_copy_key $TEST_ADMIN $TEST_ADMIN_PASSWD $TEST_USER $TEST_USER_PASSWD $KEY_TYPE
    ssh_copy_key $TEST_USER $TEST_USER_PASSWD $TEST_ADMIN $TEST_ADMIN_PASSWD $KEY_TYPE

    # check if key created with correct permissions and SELinux context
    ssh_check_home $TEST_ADMIN $TEST_ADMIN_PASSWD $KEY_TYPE $KEY yes
    ssh_check_home $TEST_USER $TEST_USER_PASSWD $KEY_TYPE $KEY yes

    # test successful connection without password
    AUDITMARK=$(get_audit_mark)
    ssh_connect_key $TEST_ADMIN $TEST_ADMIN_PASSWD $TEST_USER || \
        exit_fail "Failed to connect from $TEST_ADMIN to $TEST_USER"
    ssh_check_audit $AUDITMARK
    AUDITMARK=$(get_audit_mark)
    ssh_connect_key $TEST_USER $TEST_USER_PASSWD $TEST_ADMIN || \
        exit_fail "Failed to connect from $TEST_USER to $TEST_ADMIN"
    ssh_check_audit $AUDITMARK

    # test that connection cannot be established with an unknown key
    # the ssh connection should ask for password
    ssh_cleanup_home $TEST_ADMIN $TEST_ADMIN_PASSWD $KEY_TYPE
    ssh_create_key $TEST_ADMIN $TEST_ADMIN_PASSWD $KEY_TYPE $KEY | \
        egrep "$KEY_TYPE_CAP[[:space:]]+$KEY" || \
            exit_fail "Failed to create $KEY_TYPE $KEY key"
    ssh_connect_key $TEST_ADMIN $TEST_ADMIN_PASSWD $TEST_USER; RET=$?
    [ $RET -eq 0 ] && exit_fail "Connection succeeded with invalid key"
    [ $RET -ne 2 ] && exit_error "Connecting did not fail as expected"
done

# try with TEST_ADMIN to access TEST_USER .ssh folder and .ssh/authorized keys
ssh_cmd $TEST_ADMIN $TEST_ADMIN_PASSWD \
    "ls /home/$TEST_USER/.ssh" && exit_error \
        "Successful listing of other user's .ssh folder"
ssh_cmd $TEST_ADMIN $TEST_ADMIN_PASSWD \
    "echo FAKEKEY >> /home/$TEST_USER/.ssh/authorized_keys" && exit_error \
        "Successful change of other user's authorized_keys file"

exit_pass
