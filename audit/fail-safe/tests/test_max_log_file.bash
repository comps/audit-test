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

action=$1	# ignore, syslog, suspend, rotate, keep_logs
max_log_file=1	# 1 megabyte, that is

write_auditd_conf \
    num_logs=2 \
    max_log_file=$max_log_file \
    max_log_file_action=$action

# Prepopulate log with max_log_file minus 10k
max_log_file=$max_log_file perl -e '
    $mystring = sprintf "%-1023s\n", "type=AGRIFFIS";
    for ($x = 0; $x < $ENV{max_log_file} * 1024 - 10; $x++) {
	print $mystring;
    }' >"$audit_log"
stat "$audit_log"

if [[ $(type -t pre_$action) == function ]]; then
    pre_$action
fi

service auditd start

# each record is at least 150 bytes (based on empirical evidence), so writing
# 100 records should always take us over (150 * 100 =~ 14k)
write_records 100

check_$action
