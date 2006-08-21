#!/bin/sh

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
# syscall_attrs.sh - This filter test is designed to test the ability to use
#                    auditctl to specify a kernel filter for the audit logs.
#                    Specifically it tests:
#           Test 1 - The ability to filter on syscall success
#           Test 2 - The ability to filter on syscall failure
#           Test 3 - The ability to filter on a syscall by name
#           Test 4 - The ability to filter on a syscall by number


#
# configuration
#

source filter_functions.bash

# used in auditctl (i.e., auditctl -[a|d] $filter_rule)
filter_rule="exit,always -S open"

#
# helper functions
#

# override the test harness cleanup function
prepend_cleanup '
    # remove the filter we set earlier
    auditctl -d $filter_rule $filter_field 2>/dev/null'

#
# main
#

# return value
ret_val=0

# create the test files
chmod 0600 $tmp1 || exit_error "unable to set the permissions on the test file"
chown root:root $tmp1 || exit_error "unable to set the permissions on the test file"

# get syscall information
syscall_name="open"
syscall_num="$(augrok --resolve $syscall_name)"
[ "$syscall_num" = "" ] && exit_error "unable to determine the syscall number for $syscall_name"

### Test 1 - Filter on syscall success

# set an audit filter
filter_field="-F success=1"
auditctl -a $filter_rule $filter_field

# audit log marker
log_mark=$(stat -c %s $audit_log)

# generate an audit event
do_open_file $tmp1

# check for the audit record
augrok --seek=$log_mark "syscall==$syscall_num" "name==$tmp1" "success==yes"
ret_val_tmp=$?
[ "$ret_val" = "0" ] && ret_val=$ret_val_tmp

# display the result
if [ "$ret_val_tmp" = "0" ]; then
    echo "notice: found audit record - PASS"
else
    echo "notice: did not find audit record - FAIL"
fi

# remove the filter
auditctl -d $filter_rule $filter_field
filter_field=""


### Test 2 - Filter on syscall failure

# set an audit filter
filter_field="-F success!=1"
auditctl -a $filter_rule $filter_field

# audit log marker
log_mark=$(stat -c %s $audit_log)

# generate an audit event
do_open_file $tmp1 "fail"

# check for the audit record
augrok --seek=$log_mark "syscall==$syscall_num" "name==$tmp1" "success==no"
ret_val_tmp=$?
[ "$ret_val" = "0" ] && ret_val=$ret_val_tmp

# display the result
if [ "$ret_val_tmp" = "0" ]; then
    echo "notice: found audit record - PASS"
else
    echo "notice: did not find audit record - FAIL"
fi

# remove the filter
auditctl -d $filter_rule $filter_field
filter_field=""


### Test 3 - Filter on the syscall by name

# set an audit filter
filter_rule="exit,always -S open"
auditctl -a $filter_rule

# audit log marker
log_mark=$(stat -c %s $audit_log)

# generate an audit event
do_open_file $tmp1

# check for the audit record
augrok --seek=$log_mark "name==$tmp1" "syscall==$syscall_num"
ret_val_tmp=$?
[ "$ret_val" = "0" ] && ret_val=$ret_val_tmp

# display the result
if [ "$ret_val_tmp" = "0" ]; then
    echo "notice: found audit record - PASS"
else
    echo "notice: did not find audit record - FAIL"
fi

# remove the filter
auditctl -d $filter_rule


### Test 4 - Filter on the syscall by number

# set an audit filter
filter_rule="exit,always -S $syscall_num"
auditctl -a $filter_rule

# audit log marker
log_mark=$(stat -c %s $audit_log)

# generate an audit event
do_open_file $tmp1

# check for the audit record
augrok --seek=$log_mark "name==$tmp1" "syscall==$syscall_num"
ret_val_tmp=$?
[ "$ret_val" = "0" ] && ret_val=$ret_val_tmp

# display the result
if [ "$ret_val_tmp" = "0" ]; then
    echo "notice: found audit record - PASS"
else
    echo "notice: did not find audit record - FAIL"
fi

# remove the filter
auditctl -d $filter_rule

exit $ret_val
