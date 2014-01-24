#!/bin/bash
###############################################################################
#   Copyright (c) 2014 Red Hat, Inc. All rights reserved.
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
# Reference SFR:
#  FCS_CKM.2(NET)
#
# AUTHOR:
#  Miroslav Vadkerti <mvadkert@redhat.com>
#
# DESCRIPTION:
#  This test tries out all supported DH key exchange algorithms and verifies
#  successful login with all of them. It also checks if two incompatible
#  key exchange algorithms cause the connection to fail. For successful logins
#  also audit log is checked for containing correct CRYPTO_SESSION events.
#
# IMPORTANT:
#  During the test the sleep command from /etc/profile is removed and
#  it is restored after the test.
#

source testcase.bash || exit 2
source tp_ssh_functions.bash || exit 2

#### main ####
KEXS="curve25519-sha256@libssh.org ecdh-sha2-nistp256 \
ecdh-sha2-nistp384 ecdh-sha2-nistp521 diffie-hellman-group-exchange-sha256 \
diffie-hellman-group-exchange-sha1 diffie-hellman-group14-sha1 \
diffie-hellman-group1-sha1"

KEXS="diffie-hellman-group1-sha1 diffie-hellman-group14-sha1 \
diffie-hellman-group-exchange-sha1 diffie-hellman-group-exchange-sha256 \
ecdh-sha2-nistp256 ecdh-sha2-nistp384"
MPROFILE="/etc/profile"
CCCONF="/etc/profile.d/cc-configuration.sh"
SSHDCONF="/etc/ssh/sshd_config"

# be verbose
set -x

# enable sysadm_u login via ssh
setsebool ssh_sysadm_login=1
append_cleanup "setsebool ssh_sysadm_login=0"

# remove sleep and remove SSH_USE_STRONG_RNG from environment
disable_ssh_strong_rng

# cleanup .ssh folders
prepend_cleanup "ssh_cleanup_home $TEST_USER $TEST_USER_PASSWD"
prepend_cleanup "ssh_cleanup_home $TEST_ADMIN $TEST_ADMIN_PASSWD"

# Try all kex algorithms for connection
for KEX in $KEXS; do
    AUDITMARK=$(get_audit_mark)
    # try to connect with password authentication
    ssh_connect_pass $TEST_ADMIN $TEST_ADMIN_PASSWD $TEST_USER \
        $TEST_USER_PASSWD "-oKexAlgorithms=$KEX" || \
        exit_fail "Failed to connect from $TEST_ADMIN to $TEST_USER"
    # check correct CRYPTO_SESSION event - check only last two records (first two are from the connection to $TEST_ADMIN
    augrok --seek=$AUDITMARK type==CRYPTO_SESSION \
        msg_1=~"op=start direction=from-server.*pfs=$KEX.*lport=22.*" || \
        exit_fail "Could not find expected CRYPTO_SESSION record from-server"
    augrok --seek=$AUDITMARK type==CRYPTO_SESSION \
        msg_1=~"op=start direction=from-client.*pfs=$KEX.*lport=22.*" || \
        exit_fail "Could not find expected CRYPTO_SESSION record from-client"
done

# Try incompatible kex algorithms
sshd_config_set "KexAlgorithms" "diffie-hellman-group1-sha1"

# try to connect with password authentication
ssh_connect_pass $TEST_ADMIN $TEST_ADMIN_PASSWD $TEST_USER $TEST_USER_PASSWD \
    "-oKexAlgorithms=ecdh-sha2-nistp256"; RET=$?
[ $RET -eq 0 ] && exit_fail \
    "Connection successful with incompatible key exchange methods"
[ $RET -ne 3 ] && exit_error "Unexpected error message"

exit_pass
