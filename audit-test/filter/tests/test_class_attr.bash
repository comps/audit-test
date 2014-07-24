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
# Test the ability to filter by the attr syscall class.  This test verifies two
# syscalls that are part of the 'attr' class, and one syscall that is not.

source filter_functions.bash || exit 2

# setup

watch=$tmp1

auditctl -a exit,always -F path=$watch -F perm=a
prepend_cleanup "auditctl -d exit,always -F path=$watch -F perm=a"

log_mark=$(stat -c %s $audit_log)

# test
do_chmod $watch 777
if [[ ${MACHINE} = "aarch64" ]]; then
    do_fchownat $(dirname $watch) $(basename $watch) root
else
    do_chown $watch root
fi
do_unlink $watch

# verify audit record
if [[ ${MACHINE} = "aarch64" ]]; then
    augrok --seek=$log_mark type==SYSCALL syscall==fchmodat name==$watch \
        || exit_fail "Expected record for 'chmod' not found."
    augrok --seek=$log_mark type==SYSCALL syscall==fchownat
           name==$(basename $watch) \
        || exit_fail "Expected record for 'chown' not found."
    augrok --seek=$log_mark type==SYSCALL syscall==unlinkat name==$watch \
        && exit_fail "Unexpected record for 'unlink' found."
else
    augrok --seek=$log_mark type==SYSCALL syscall==chmod name==$watch \
        || exit_fail "Expected record for 'chmod' not found."
    augrok --seek=$log_mark type==SYSCALL syscall==chown name==$watch \
        || exit_fail "Expected record for 'chown' not found."
    augrok --seek=$log_mark type==SYSCALL syscall==unlink name==$watch \
        && exit_fail "Unexpected record for 'unlink' found."
fi

exit_pass
