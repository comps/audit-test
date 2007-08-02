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
# Verify max_log_file and max_log_file_action auditd configuration items are
# effective.  Test all possible values of max_log_file_action: ignore, syslog,
# suspend, rotate, keep_logs

source auditd_common.bash || exit 2

max_log_file=1	# 1 megabyte, that is

write_config -s "$auditd_conf" \
    num_logs=2 \
    max_log_file=$max_log_file \
    max_log_file_action=$action || exit 2

restart_auditd || exit 2

# Prepopulate log with max_log_file minus 5k
write_file "$audit_log" $((max_log_file * 1024 - 5)) || exit 2

# each record is at least 80 bytes (based on empirical evidence), so writing
# 200 records should always take us over (200 * 80 =~ 15k)
write_records 200 || exit 2

case $action in
    email)
	check_$action "Subject: FIXME" ;;
    syslog)
        check_$action "Audit daemon log file is larger than max size" ;;
    suspend)
        check_$action "Audit daemon is suspending logging due to logfile size" ;;
    *)
        check_$action ;;
esac
exit $?
