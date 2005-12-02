#!/bin/bash
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
# Verify disk_full_action auditd configuration item is effective.
# Test all possible values of disk_full_action: ignore,
# syslog, suspend, single, halt

source auditd_common.bash

write_auditd_conf \
    disk_full_action=$action || exit 2

# Fill the filesystem hosting audit.log, leaving 10k available
fill_disk ${audit_log%/*} 10 || exit 2

start_auditd || exit 2

# each record is at least 150 bytes (based on empirical evidence), so writing
# 100 records should always take us over (100 * 150 =~ 14k)
write_records 100 || exit 2

case $action in
    syslog)
        check_$action "Audit daemon has no space left on logging partition" ;;
    *)
        check_$action ;;
esac
exit $?
