#!/bin/bash
# =============================================================================
# (c) Copyright Hewlett-Packard Development Company, L.P., 2005
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
# PURPOSE:
# Verify audit of changes to system time.

HWCLOCK="/sbin/hwclock"
AUDIT_LOG="/var/log/audit/audit.log"

# Fetch the UTC setting from the system clock configuration
source /etc/sysconfig/clock
case $UTC in
    true|yes) UTCFLAG=--utc ;;
    *) unset UTCFLAG ;;
esac

## Test 1
#
# Change the system time and check for the audit event:
# type=USYS_CONFIG msg=audit(x.y): user pid=n uid=0 auid=m
#       msg='hwclock: op=changing system time id=0 res=success'

# Store off the audit log offset
AUDIT_SEEK=$(wc -c < $AUDIT_LOG)

# Alter the hardware clock
echo "$(hwclock) -- original hardware clock"
$HWCLOCK --set --date "1/1/2000 00:00:00"
echo "$(hwclock) -- altered hardware clock"

# Restore the hwclock
$HWCLOCK $UTCFLAG --systohc
echo "$(hwclock) -- restored hardware clock"

# Check for the records
count=$(augrok --count --seek $AUDIT_SEEK type==USYS_CONFIG \
    msg_1=~"changing system time: exe=./sbin/hwclock.*res=success.*")
if [[ $count == 2 ]]; then
    echo "pass: augrok found 2 hwclock records"
    exit 0
else
    echo "fail: augrok found $count records, expecting only 2"
    exit 1
fi
