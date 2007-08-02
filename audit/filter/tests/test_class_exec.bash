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
# Test the ability to filter by the exec syscall class.  This test verifies one
# syscall that is part of the 'exec' class, and one syscall that is not.

source filter_functions.bash || exit 2

# setup

# use syscall utilities
PATH=$TOPDIR/utils:$TOPDIR/utils/bin:$PATH

watch=$tmp1

auditctl -a exit,always -w $watch -p x
prepend_cleanup "auditctl -d exit,always -w $watch -p x"

log_mark=$(stat -c %s $audit_log)

# test
do_execve $watch
do_unlink $watch

# verify audit record
augrok --seek=$log_mark type==SYSCALL syscall==execve name==$watch \
    || exit_fail "Expected record for 'execve' not found."
augrok --seek=$log_mark type==SYSCALL syscall==unlink name==$watch \
    && exit_fail "Unexpected record for 'unlink' found."

exit_pass
