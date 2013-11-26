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
# SFRs: FCS_CKM.1(SYM), FCS_CKM.2(NET)
#
# AUTHOR: Miroslav Vadkerti <mvadkert@redhat.com>
#
# DESCRIPTION:
# Connect between 2 users using all required ciphers:
# + aes128-cbc (128 bit)
# + aes192-cbc (192 bit)
# + aes256-cbc (256 bit)
# + 3dec-cbc (160 bit)
# Test if sshd reads 48bits from /dev/random on client connect
# Also checks for correct audit logs logged by ssh daemon.
#

source testcase.bash || exit 2
source tp_ssh_functions.bash || exit 2

#### main ####

# globals
CIPHERS="aes128-cbc 3des-cbc aes192-cbc aes256-cbc"
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

# generate a little entropy with rngd to PASS the tests on low entropy
# systems
rngd -r /dev/urandom
prepend_cleanup "killall rngd"

# restart sshd to get default state with SSH_USE_STRONG_RNG enabled
restart_service sshd

# get the pid of sshd process running on port 22
SSHDPID=$(ss -4 -ltnp | grep sshd | sed 's/.*sshd",\([0-9]\+\),.*/\1/')
[ -z "$SSHDPID" ] && exit_error "could not find sshd process pid"

# check if SSH_USE_STRONG_RNG set in environemnt of the sshd process
grep "SSH_USE_STRONG_RNG" /proc/$SSHDPID/environ
[ $? -ne 0 ] && exit_fail "SSH_USE_STRONG_RNG not exported for sshd"

# check if data read from /dev/random if client connects
SOUT=$(mktemp)
prepend_cleanup "rm -f $SOUT"
strace -fp $SSHDPID &> $SOUT &
prepend_cleanup "pkill -9 strace"
ssh_connect_pass $TEST_ADMIN $TEST_ADMIN_PASSWD $TEST_USER \
    $TEST_USER_PASSWD || \
    exit_fail "Failed to connect from $TEST_USER to $TEST_ADMIN"
egrep -A4 "open.*/dev/random" $SOUT | grep read &> /dev/null
[ $? -ne 0 ] && exit_fail "sshd failed to read /dev/random"

# remove SSH_USE_STRONG_RNG from environment
ssh_remove_strong_rng_env
# remove SSH_USE_STRONG_RNG from files
ssh_remove_strong_rng $SSHDCONF
ssh_remove_strong_rng $CCCONF
restart_service sshd

# connect using all ciphers with password authentication
for CIPHER in $CIPHERS; do
    # test connection without password with
    AUDITMARK=$(get_audit_mark)
    ssh_connect_pass $TEST_ADMIN $TEST_ADMIN_PASSWD $TEST_USER \
        $TEST_USER_PASSWD "-c $CIPHER" || \
        exit_fail "Failed to connect from $TEST_ADMIN to $TEST_USER"
    ssh_check_audit $AUDITMARK
    AUDITMARK=$(get_audit_mark)
    ssh_connect_pass $TEST_USER $TEST_USER_PASSWD $TEST_ADMIN \
        $TEST_ADMIN_PASSWD "-c $CIPHER" || \
        exit_fail "Failed to connect from $TEST_USER to $TEST_ADMIN"
    ssh_check_audit $AUDITMARK
done

exit_pass
