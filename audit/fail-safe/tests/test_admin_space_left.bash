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
# Verify admin_space_left and admin_space_left_action auditd configuration items
# are effective.  Test all possible values of admin_space_left_action: ignore,
# syslog, email, suspend, single, halt

source auditd_common.bash

write_auditd_conf \
    admin_space_left=7 \
    admin_space_left_action=$action \
    space_left=8 || exit 2

# Fill the filesystem hosting audit.log, leaving 1MB + 10KB available
fill_disk ${audit_log%/*} $((1024 + 10)) || exit 2

start_auditd || exit 2

# each record is at least 150 bytes (based on empirical evidence), so writing
# 100 records should always take us over (150 * 100 =~ 14k)
write_records 100 || exit 2

case $action in
    email)
	check_$action "Subject: Audit Admin Space Alert" ;;
    syslog)
        check_$action "Audit daemon is low on disk space for logging" ;;
    *)
        check_$action ;;
esac
exit $?
