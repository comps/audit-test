#!/bin/bash
###############################################################################
# (c) Copyright Hewlett-Packard Development Company, L.P., 2011
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
# Test the ability to filter on type of audit event
#
# For type, we can only exclude auditing specific types, not specifically 
# audit them, so this test works in reverse
# First, verify that the record is found by default
# Second, exclude it

source filter_functions.bash || exit 2

# setup
user_auid=$(cat /proc/self/loginuid)

# setup auditctl
auditctl -a exit,always -F arch=b$MODE -S open -F auid=$user_auid
prepend_cleanup "auditctl -d exit,always -F arch=b$MODE -S open -F auid=$user_auid"

# audit log marker
log_mark=$(stat -c %s $audit_log)

# generate an audit event
do_open_file $tmp1

# should find it
augrok --seek=$log_mark "name==$tmp1" "auid==$user_auid" type==PATH\
    || exit_error "Expected record not found."

# exclude the syscall record type
auditctl -a exclude,always -F msgtype=PATH
prepend_cleanup "auditctl -d  exclude,always -F msgtype=PATH"

# audit log marker
log_mark=$(stat -c %s $audit_log)

# generate an audit event
do_open_file $tmp1

# shouldn't find it
augrok --seek=$log_mark "name==$tmp1" "auid==$user_auid" "type==PATH" \
    && exit_fail "Unexpected record found."

exit_pass
