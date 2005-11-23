#!/bin/bash -e
# =============================================================================
# (c) Copyright Hewlett-Packard Development Company, L.P., 2005
# Written by Aron Griffis <aron@hp.com>
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
#
# PURPOSE:
# Verify admin_space_left and admin_space_left_action auditd configuration items
# are effective.  Test all possible values of admin_space_left_action: ignore,
# syslog, email, suspend, single, halt

source $(dirname "$0")/auditd_common.bash

action=$1       	# ignore, syslog, email, suspend, single, halt
admin_space_left=7	# 7 megabytes, that is

write_auditd_conf \
    admin_space_left=$admin_space_left \
    admin_space_left_action=$action \
    space_left=$((admin_space_left + 1))

# Fill up the filesystem, leaving slightly more than space_left available
dd if=/dev/zero of=${audit_log%/*}/bogus bs=1024 count=1014
df -k ${audit_log%/*}

if [[ $(type -t pre_$action) == function ]]; then
    pre_$action
fi

service auditd start

# each record is at least 150 bytes (based on empirical evidence), so writing
# 100 records should always take us over (150 * 100 =~ 14k)
write_records 100
df -k ${audit_log%/*}

check_$action
