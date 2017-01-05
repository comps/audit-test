#!/bin/bash
###############################################################################
# (c) Copyright Hewlett-Packard Development Company, L.P., 2006
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
###############################################################################
#
# PURPOSE:
# Test the ability to filter on syscall success or failure.

source filter_functions.bash || exit 2

# setup
if [[ ${MACHINE} = "aarch64" ]]; then
    syscall_name="openat"
else
    syscall_name="open"
fi
syscall_num=$(augrok --resolve $syscall_name) \
    || exit_error "unable to determine the syscall number for $syscall_name"

op=$1
case $op in
    yes)
        gen_audit_event="do_open_file $tmp1"
        filter_field="-F success=1"
        ;;
    no)
        gen_audit_event="do_open_file $tmp1 fail"
        filter_field="-F success!=1"
        ;;
    *) exit_fail "unknown test operation" ;;
esac
filter_rule="exit,always -F arch=b$MODE -S $syscall_name"

auditctl -a $filter_rule $filter_field
prepend_cleanup "auditctl -d $filter_rule $filter_field"

# audit log marker
log_mark=$(stat -c %s $audit_log)

# generate an audit event
eval "$gen_audit_event"

wait_for_cmd "augrok --seek=\"$log_mark\" \
              syscall==\"$syscall_num\" name==\"$tmp1\" success==\"$op\"" || \
    exit_fail "Expected record not found."

exit_pass
