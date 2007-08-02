#!/bin/bash
# =============================================================================
# (c) Copyright Hewlett-Packard Development Company, L.P., 2005
# Written by Aron Griffis <aron@hp.com>
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
# Verify admin_space_left and admin_space_left_action auditd configuration items
# are effective.  Test all possible values of admin_space_left_action: ignore,
# syslog, email, suspend, single, halt

source auditd_common.bash || exit 2

write_config -s "$auditd_conf" \
    admin_space_left=1 \
    admin_space_left_action=$action \
    space_left=2 || exit 2

restart_auditd || exit 2

# Fill the filesystem hosting audit.log, leaving 1MB + 5KB available
fill_disk ${audit_log%/*} $((1024 + 5)) || exit 2

# each record is at least 80 bytes (based on empirical evidence), so writing
# 200 records should always take us over (200 * 80 =~ 15k)
write_records 200 || exit 2

case $action in
    email)
	check_$action "Subject: Audit Admin Space Alert" ;;
    syslog)
        check_$action "Audit daemon is low on disk space for logging" ;;
    suspend)
        check_$action "Audit daemon is suspending logging due to low disk space" ;;
    *)
        check_$action ;;
esac
exit $?
