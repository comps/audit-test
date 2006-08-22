#!/bin/bash
###############################################################################
# (c) Copyright Hewlett-Packard Development Company, L.P., 2005
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
###############################################################################
#
# PURPOSE:
# Test the ability to filter on syscall name or number.

source filter_functions.bash || exit 2

# setup
syscall_name="open"
syscall_num=$(augrok --resolve $syscall_name) \
    || exit_error "unable to determine the syscall number for $syscall_name"

op=$1
case $op in
    name)   filter_rule="exit,always -S open" ;;
    number) filter_rule="exit,always -S $syscall_num";;
    *) exit_fail "unknown test operation" ;;
esac

auditctl -a $filter_rule
prepend_cleanup "auditctl -d $filter_rule"

# audit log marker
log_mark=$(stat -c %s $audit_log)

# generate an audit event
do_open_file $tmp1

augrok --seek=$log_mark "name==$tmp1" "syscall==$syscall_num" \
    || exit_fail "Expected record not found."

exit_pass
