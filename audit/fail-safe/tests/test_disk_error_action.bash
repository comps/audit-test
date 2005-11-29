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
# Verify disk_full_action auditd configuration item is effective.
# Test all possible values of disk_full_action: ignore,
# syslog, suspend, single, halt

source $(dirname "$0")/auditd_common.bash

write_auditd_conf \
    disk_error_action=$action

# start auditd manually instead of with the initscript
# so we can set LD_PRELOAD.
(
    export LD_PRELOAD="$PWD/fprintf.so $LD_PRELOAD"
    auditd || auditd -f # to capture errors in test output
)

write_records 100

case $action in
    syslog)
        check_$action "Audit daemon detected an error writing an event to disk" ;;
    *)
        check_$action ;;
esac
