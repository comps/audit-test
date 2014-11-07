#!/bin/bash
# =============================================================================
# (c) Copyright Hewlett-Packard Development Company, L.P., 2005
#  Copyright (c) 2014 Red Hat, Inc. All rights reserved.
#
#   This program is free software: you can redistribute it and/or modify
#   it under the terms of version 2 the GNU General Public License as
#   published by the Free Software Foundation.
#   
#   This program is distributed in the hope that it will be useful,
#   but WITHOUT ANY WARRANTY; without even the implied warranty of
#   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#   GNU General Public License for more details.
#   
#   You should have received a copy of the GNU General Public License
#   along with this program.  If not, see <http://www.gnu.org/licenses/>.
# =============================================================================
#
# SFRs: FPT_STM.1
#
# DESCRIPTION:
# Verify audit of changes to system time.
# Check if only privileged user is able to set hwclock.
#

source testcase.bash

HWCLOCK_UTILITY="/sbin/hwclock"
AUDIT_LOG="/var/log/audit/audit.log"

# Check if UTC timezone used
readlink /etc/localtime | grep -q UTC$
if [ $? -eq 0 ]; then
    UTCFLAG=--utc
else
    unset UTCFLAG
fi

## Test 1
#
# Change the system time and check for the audit event:
# type=USYS_CONFIG msg=audit(x.y): user pid=n uid=0 auid=m
#       msg='hwclock: op=changing system time id=0 res=success'

# Store off the audit log offset
AUDIT_SEEK=$(get_audit_mark)

# Alter the hardware clock
echo "$(hwclock) -- original hardware clock"
$HWCLOCK_UTILITY --set --date "1/1/2000 00:00:00"
echo "$(hwclock) -- altered hardware clock"

# Restore the hwclock
$HWCLOCK_UTILITY $UTCFLAG --systohc
echo "$(hwclock) -- restored hardware clock"

# Check for the records
count=$(augrok --count --seek $AUDIT_SEEK type==USYS_CONFIG \
    msg_1=~"changing system time.*exe=./usr/sbin/hwclock.*res=success.*")
if [[ $count == 2 ]]; then
    echo "pass: augrok found 2 hwclock records"
else
    exit_fail "fail: augrok found $count records, expecting only 2"
fi

# Try to set the time as untrusted user
su -c "$HWCLOCK_UTILITY --set --date '1/1/2000 00:00:00'" $TEST_USER
if [ $? -eq 0 ]; then
    # Restore HW clock if setting hwclock passed
    $HWCLOCK_UTILITY $UTCFLAG --systohc
    echo "$(hwclock) -- restored hardware clock"
    exit_fail "$TEST_USER was able to set the time"
fi

exit_pass
