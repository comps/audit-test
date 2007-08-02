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
# Verify disk_full_action auditd configuration item is effective.
# Test all possible values of disk_full_action: ignore,
# syslog, suspend, single, halt

source auditd_common.bash || exit 2

write_config -s "$auditd_conf" \
    disk_error_action=$action || exit 2

(
    # fake a disk error by preloading fprintf to fail on USER messages
    export LD_PRELOAD="$PWD/fprintf.so $LD_PRELOAD"
    echo "Preloading $LD_PRELOAD"
    # don't use start_auditd because it will send USER messages
    auditd || { auditd -f; exit 2; }
    sleep 2
) || exit $?

write_records 100 || exit 2

case $action in
    syslog)
        check_$action "write: Audit daemon detected an error writing an event to disk (Input/output error)" ;;
    *)
        check_$action ;;
esac
exit $?
