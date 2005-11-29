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
# Verify max_log_file and max_log_file_action auditd configuration items are
# effective.  Test all possible values of max_log_file_action: ignore, syslog,
# suspend, rotate, keep_logs

source $(dirname "$0")/auditd_common.bash

max_log_file=1	# 1 megabyte, that is

write_auditd_conf \
    num_logs=2 \
    max_log_file=$max_log_file \
    max_log_file_action=$action

# Prepopulate log with max_log_file minus 10k
write_file "$audit_log" $((max_log_file * 1024 - 10))

service auditd start || auditd -f  # to capture errors in test output

# each record is at least 150 bytes (based on empirical evidence), so writing
# 100 records should always take us over (150 * 100 =~ 14k)
write_records 100

case $action in
    syslog)
        check_$action "Audit daemon log file is larger than max size" ;;
    *)
        check_$action ;;
esac
