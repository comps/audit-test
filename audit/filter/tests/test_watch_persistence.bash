#!/bin/bash
###############################################################################
# (c) Copyright Hewlett-Packard Development Company, L.P., 2007
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
# Test that audit records continue for a watched file when it is removed and
# created again.

source filter_functions.bash || exit 2

# setup
tmpd=$(mktemp -d) || exit_fail "create tempdir failed"
name="$tmpd/foo"

auditctl -a exit,always -w $name

prepend_cleanup "
    auditctl -d exit,always -w $name
    rm -rf $tmpd"

# initially create the file; verify audit record
log_mark=$(stat -c %s $audit_log)
do_open $name create

augrok --seek=$log_mark type==SYSCALL syscall==open name==$name success==yes \
    || exit_fail "Expected record not found for initial create of watched file"

# remove the file; verify audit record
log_mark=$(stat -c %s $audit_log)
rm $name

augrok --seek=$log_mark type==SYSCALL syscall==unlink name==$name success==yes \
    || exit_fail "Expected record not found for removal of watched file"

# create the file again; verify audit record
log_mark=$(stat -c %s $audit_log)
do_open $name create

augrok --seek=$log_mark type==SYSCALL syscall==open name==$name success==yes \
    || exit_fail "Expected record not found for re-create of watched file"

exit_pass
