#!/bin/bash
###############################################################################
# (c) Copyright Hewlett-Packard Development Company, L.P., 2007
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
# Test the ability to filter by the read syscall class.  This test verifies two
# syscalls that are part of the 'read' class, and one syscall that is not.

source filter_functions.bash || exit 2

# setup

# use syscall utilities
PATH=$TOPDIR/utils:$TOPDIR/utils/bin:$PATH

watch=$tmp1

auditctl -a exit,always -w $watch -p r
prepend_cleanup "auditctl -d exit,always -w $watch -p r"

log_mark=$(stat -c %s $audit_log)

# test
do_listxattr $watch
do_getxattr $watch "user.mime_type"
do_truncate $watch

# verify audit record
augrok --seek=$log_mark type==SYSCALL syscall==listxattr name==$watch \
    || exit_fail "Expected record for 'listxattr' not found."
augrok --seek=$log_mark type==SYSCALL syscall==getxattr name==$watch \
    || exit_fail "Expected record for 'getxattr' not found."
augrok --seek=$log_mark type==SYSCALL syscall==truncate name==$watch \
    && exit_fail "Unexpected record for 'truncate' found."

exit_pass
