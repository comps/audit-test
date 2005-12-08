#!/bin/bash
# =============================================================================
# (c) Copyright Hewlett-Packard Development Company, L.P., 2005
#
#   This program is free software;  you can redistribute it and/or modify
#   it under the terms of the GNU General Public License as published by
#   the Free Software Foundation; either version 2 of the License, or
#   (at your option) any later version.
#
#   This program is distributed in the hope that it will be useful,
#   but WITHOUT ANY WARRANTY;  without even the implied warranty of
#   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See
#   the GNU General Public License for more details.
#
#   You should have received a copy of the GNU General Public License
#   along with this program;  if not, write to the Free Software
#   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
# =============================================================================

HWCLOCK="/sbin/hwclock"
AUDIT_LOG="/var/log/audit/audit.log"

## Test 1
#
# Change the system time and check for the audit event:
# type=USYS_CONFIG msg=audit(x.y): user pid=n uid=0 auid=m
#       msg='hwclock: op=changing system time id=0 res=success'

# Store off the audit log offset
AUDIT_SEEK=`wc -c $AUDIT_LOG | awk '{print $1}'`

# Save off the current time
ALT_TIME="1/1/00 00:00:00"
UTC_TIME=`/bin/date -u +"%D %T"`

# Set the time, then restore it
echo "Setting hwclock to   $ALT_TIME"

$HWCLOCK --set --date="$ALT_TIME"
$HWCLOCK -u --set --date="$UTC_TIME"

echo -n "System time restored "
date +"%D %T"

# Check for the records
[ `augrep --count --seek $AUDIT_SEEK type==USYS_CONFIG msg_1=="hwclock: op=changing system time id=0 res=success"` == 2 ] && exit 0;

# There weren't exactly 2 records therefor the test failed
exit 1;
