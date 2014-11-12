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
# SFR: FTA_SSL.1, FTA_SSL.2
#
# AUTHOR: Miroslav Vadkerti <mvadkert@redhat.com>
#
# PREREQUISITES:
# Configured screen according to the CCC requirements.
#
# DESCRIPTION:
# The test checks if
# + screen locked after specific amount of time
# + screen asking for password
# + unlock successful using the correct password
# + unlock fails using an incorrect password
# + screen asks again for the password if attempt failed
# + escape sequence for clearing the screen sent if screen locked
# + kernel boot options contain "no-scroll" and "fbcon=scrollback:0"
# + screen locking via "CTRL-A x" works
# + control sequences (CTRL-C, CTRL-D, CTRL-A d)
#   do not work when screen locked
#
# NOTE:
# During the test the sleep command from /etc/profile is removed
# so the test can run in a reasonable time frame. The changed
# file is restored after the testing.
#

source testcase.bash || exit 2
source tp_screen_functions.bash || exit 2

#### main ####

# globals
PROFILE="/etc/profile"

# be verbose
set -x

# enable IPA
source $TOPDIR/utils/auth-server/ipa_env
restart_service sssd
prepend_cleanup "stop_service sssd"

# make sure test user is not locked after testing
prepend_cleanup "faillock --reset --user $TEST_USER"

# make sure TEST_USER and TEST_ADMIN leave no trace in the system
append_cleanup "kill -9 $(lsof | egrep "($TEST_USER|$TEST_ADMIN)" | \
    awk '{print $2}' | sort | uniq)"

# check permissions of global configuration
screen_check_global_config

# backup global profile and remove sleep
backup $PROFILE
screen_remove_sleep $PROFILE

# check if
# + screen locked after LOCK_TIME seconds
# + screen asking for password
# + unlock successful using the correct password
for LOCK_TIME in 2 3 4 6; do
    # local authentication
    screen_config_set_lock $SCREENRC $LOCK_TIME
    screen_check_lock $TEST_USER $TEST_USER_PASSWD $LOCK_TIME || \
        exit_fail "screen for $TEST_USER did not lock after 2s ($?)"
    # remote authentication
    screen_config_set_lock $SCREENRC $LOCK_TIME
    screen_check_lock $IPA_USER $IPA_PASS $LOCK_TIME || \
        exit_fail "screen for $IPA_USER did not lock after 2s ($?)"
done

# set test user lockscreen timeout to 2s
su - -c "echo 'idle 2 lockscreen' > ~/.screenrc" $TEST_USER || \
    exit_fail "Failed to set lockscreen timeout for $TEST_USER"

# check if
# + unlock using a incorrect password fails
# + screen asks again for the password if attempt failed
screen_check_badpass $TEST_USER $TEST_USER_PASSWD 2 || exit_fail \
    "screen unlock with incorrect password failed unexpectedly ($?)"

# check if
# + escape sequence for clearing the screen sent if screen locked
# + kernel boot options contain "no-scroll" and "fbcon=scrollback:0"
for LOCK_TIME in 2 3 4 6; do
    screen_check_clear $TEST_USER $TEST_USER_PASSWD $LOCK_TIME || exit_fail \
        "screen clear before locking failed ($?)"
done

# checks if
# + screen locking via "CTRL-A x" works
# + control sequences (CTRL-C, CTRL-D, CTRL-A d)
#   do not work when screen locked
screen_check_sequences $TEST_USER $TEST_USER_PASSWD || exit_fail \
    "locked screen reacts on escape sequences ($?)"

# all tests successful
exit_pass
