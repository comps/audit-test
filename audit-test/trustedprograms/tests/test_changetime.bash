#!/bin/bash
# =============================================================================
# Copyright (c) 2014 Red Hat, Inc.
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of version 2 the GNU General Public License as
# published by the Free Software Foundation.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.
# =============================================================================
#
# SFR: FPT_STM.1
#
# Author: Matus Marhefka <mmarhefk@redhat.com>
#         Miroslav Vadkerti <mvadkert@redhat.com>
#
# DESCRIPTION:
# The test uses timedatectl(1) and date(1) utilities to change the system time
# and checks if the time was set/not-set correctly:
# 1) for trusted user (root) (should pass)
# 2) for untrusted user (should fail)
#
# IMPORTANT NOTE:
# The test changes system time and tries to restore the original time ASAP
# to avoid large clock skew. The clock skew depends e < 1s in the worst scenario.
#

source testcase.bash || exit 2

# be verbose
set -x

# test time
TEST_TIME_SEC="1000000000"
TEST_TIME=$(date -d @$TEST_TIME_SEC +"%Y-%m-%d %T")

# print current time or time given in seconds from epoch
print_time() {
    [ -z "$1" ] && date +"%Y-%m-%d %T.%N" || date -d@$1 +"%Y-%m-%d %T.%N"
}

# save time for restoration
save_time() {
    SAVED_TIME=$(date +%s.%N)
}

# restore time to save_time + given time in seconds
# as first argument
restore_time() {
    # restore time
    date -s @$SAVED_TIME
}

# Set the time to TEST_TIME and restore the saved time ASAP - the change in time should be <1s
# Save the time set in SET_TIME variable (for correctness checking if needed)
testcase() {
    local RET

    [ -z "$1" ] && exit_error "No user given for change time"

    # save time for restoration
    echo "Saving time: $(print_time)"
    save_time

    su -c "$DCMD" $1; RET=$?
    SET_TIME=$(date +%s)

    # restore time
    restore_time
    echo "Restored time: $(print_time $SAVED_TIME)"

    return $RET
}

case $1 in
    timedatectl)
        if [ "$PPROFILE" = "lspp" ]; then
            DCMD="runcon -r sysadm_r -t sysadm_t timedatectl set-time '$TEST_TIME' --no-ask-password"
        else
            DCMD="timedatectl set-time '$TEST_TIME' --no-ask-password"
        fi
        ;;
    date)
        DCMD="date -s @$TEST_TIME_SEC"
        ;;
    *)
        echo "Unknown test"
        exit 1
        ;;
esac

echo "Trying to change time via $1 as root"
testcase root || exit_fail "Failed to set time as root via \"$DCMD\" command"
[ $SET_TIME -ne $TEST_TIME_SEC ] && exit_fail "$1 didn't set time correctly"


echo "Trying to change time via $1 as $TEST_USER"
testcase $TEST_USER && exit_fail "Unprivileged $TEST_USER was able to set time"

exit_pass
