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
# process_attrs.sh - This filter test is designed to test the ability to use
#                    auditctl to specify a kernel filter for the audit logs.
#                    Specifically it tests:
#           Test 1 - The ability to filter based on login uid (auid)

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
    [ -n "$filter_field" ] && auditctl -d $filter_rule $filter_field 2>/dev/null'

#
# main
#

# return value
ret_val=0

# get user information
user_auid="$(cat /proc/self/loginuid)"

### Test 1 - Filter on login uid (auid)

# set an audit filter
filter_field="-F auid=$user_auid"
auditctl -a $filter_rule $filter_field

# audit log marker
log_mark=$(stat -c %s $audit_log)

# generate an audit event
do_open_file $tmp1

# check for the audit record
augrok --seek=$log_mark "name==$tmp1" "auid==$user_auid"
ret_val=$?

exit $ret_val
