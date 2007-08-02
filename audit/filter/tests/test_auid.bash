#!/bin/bash
###############################################################################
# (c) Copyright Hewlett-Packard Development Company, L.P., 2005
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
# Test the ability to filter on login uid (auid).

source filter_functions.bash || exit 2

# setup
user_auid=$(cat /proc/self/loginuid)

auditctl -a exit,always -S open -F auid=$user_auid
prepend_cleanup "auditctl -d exit,always -S open -F auid=$user_auid"

# audit log marker
log_mark=$(stat -c %s $audit_log)

# generate an audit event
do_open_file $tmp1

augrok --seek=$log_mark "name==$tmp1" "auid==$user_auid" \
    || exit_fail "Expected record not found."

exit_pass
