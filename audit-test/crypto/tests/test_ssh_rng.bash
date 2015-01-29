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
# SFRs:
#
# AUTHOR: Miroslav Vadkerti <mvadkert@redhat.com>
#
# DESCRIPTION:
# Test if sshd reads 48bits from /dev/random on client connect
# Also checks for correct audit logs logged by ssh daemon.
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

# backup global profile and remove sleep
backup $MPROFILE
backup $SSHDCONF
backup $CCCONF
ssh_remove_screen $MPROFILE

# set SSH_USE_STRONG_RNG in config
if egrep -q "^[^#]*SSH_USE_STRONG_RNG" $SSHDCONF; then
    sed -i 's/^[^#]*SSH_USE_STRONG_RNG.*/SSH_USE_STRONG_RNG=12/' $SSHDCONF
else
    echo "SSH_USE_STRONG_RNG=12" >> $SSHDCONF
fi

# restart sshd to get default state with SSH_USE_STRONG_RNG enabled
restart_service sshd

# get the pid of sshd process running on port 22
eval $(systemctl show --property=MainPID sshd)
[ -z "MainPID" ] && exit_error "could not find sshd process pid"

# check if SSH_USE_STRONG_RNG set in environemnt of the sshd process
grep "SSH_USE_STRONG_RNG" /proc/$MainPID/environ
[ $? -ne 0 ] && exit_fail "SSH_USE_STRONG_RNG not exported for sshd"

# check if data read from /dev/random if client connects
SOUT=$(mktemp)
prepend_cleanup "rm -f $SOUT"
strace -fp $MainPID &> $SOUT &
prepend_cleanup "pkill -9 strace"
ssh_connect_pass $TEST_ADMIN $TEST_ADMIN_PASSWD $TEST_USER \
    $TEST_USER_PASSWD || \
    exit_fail "Failed to connect from $TEST_USER to $TEST_ADMIN"
egrep -A4 "open.*/dev/random" $SOUT | grep read &> /dev/null
[ $? -ne 0 ] && exit_fail "sshd failed to read /dev/random"

exit_pass
