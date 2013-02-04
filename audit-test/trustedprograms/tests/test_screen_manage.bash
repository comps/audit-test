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
# SFR: FMT_MTD.1(SSL)
#
# AUTHOR: Miroslav Vadkerti <mvadkert@redhat.com>
#
# PREREQUISITES:
# Configured screen according to the CCC requirements.
#
# DESCRIPTION:
# Test if permissions correctly restrict screen configuration and if
# screen locking works as expected according to the global/user
# configuration. Also test if administrator is able to restrict
# user configuration of screen.
#
# NOTE:
# During the test the sleep command from /etc/profile is removed and
# the default idle lockscreen timeout is changed so the test can run
# in a reasonable time frame. All changed configs are restored after
# the testing.
#

source testcase.bash || exit 2
source tp_screen_functions.bash || exit 2

#### main ####

# globals
SCREENRC="/etc/screenrc"
PROFILE="/etc/profile"

# be verbose
set -x

# make sure TEST_USER and TEST_ADMIN leave no trace in the system
append_cleanup "kill -9 $(lsof | egrep "($TEST_USER|$TEST_ADMIN)" | \
    awk '{print $2}' | sort | uniq)"

# check permissions
screen_check_global_config

# backup global profile
backup $PROFILE

# remove screen from global profile
screen_remove_sleep $PROFILE

# backup global screen config
backup $SCREENRC

# set global idle lockscreen timeout to 5s
screen_config_set_lock $SCREENRC 5

# set test user lockscreen timeout to 2s
su - -c "echo 'idle 2 lockscreen' > ~/.screenrc" $TEST_USER || \
    exit_fail "Failed to set lockscreen timeout for $TEST_USER"

# try to modify test admin screenrc with test user
su - -c "echo 'idle 30 lockscreen' > /home/$TEST_ADMIN/.screenrc" \
    $TEST_USER && exit_fail "$TEST_USER can modify ${TEST_ADMIN}'s screenrc"

# check if test user account locked after 2 seconds
screen_check_lock $TEST_USER $TEST_USER_PASSWD 2 || \
    exit_fail "screen for $TEST_USER did not lock after 2s"

# check if test admin account locked after 5 seconds
screen_check_lock $TEST_ADMIN $TEST_ADMIN_PASSWD 5 || \
    exit_fail "screen for $TEST_ADMIN did not lock after 5s"

# check if test user account wasn't locked after 2 seconds if
# screen is run with -c /dev/null to ignore user settings
screen_check_lock $TEST_USER $TEST_USER_PASSWD 2 "-c /dev/null" && \
    exit_fail "screen for $TEST_USER was locked after 2s"

# all tests successful
exit_pass
