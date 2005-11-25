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
# Verify space_left and space_left_action auditd configuration items are
# effective.  Test all possible values of space_left_action: ignore, syslog,
# email, suspend, single, halt

source $(dirname "$0")/auditd_common.bash

write_auditd_conf \
    space_left=7 \
    space_left_action=$action \
    admin_space_left=0

# Fill the filesystem hosting audit.log, leaving 1MB + 10KB available
fill_disk ${audit_log%/*} $((1024 + 10))

service auditd start || auditd -f  # to capture errors in test output

# each record is at least 150 bytes (based on empirical evidence), so writing
# 100 records should always take us over (150 * 100 =~ 14k)
write_records 100

check_$action
