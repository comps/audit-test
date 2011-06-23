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
# Test the ability to filter for objects created in a watched directory.

source filter_functions.bash || exit 2

# setup
op=$1
opat="${op}at"

tmpd=$(mktemp -d) || exit_fail "create tempdir failed"
watch="$tmpd"
name="$tmpd/foo"

auditctl -a exit,always -F arch=b64 -S $op -F path=$watch
auditctl -a exit,always -F arch=b64 -S $opat -F path=$watch

prepend_cleanup "
    auditctl -d exit,always -F arch=b64 -S $op -F path=$watch
    auditctl -d exit,always -F arch=b64 -S $opat -F path=$watch
    rm -rf $tmpd"

case $op in
    link)    gen_audit_event="ln $tmp1 $name" ;;
    mkdir)   gen_audit_event="mkdir $name" ;;
    open)    gen_audit_event="touch $name" ;;
    rename)  gen_audit_event="mv $tmp1 $name" ;;
    symlink) gen_audit_event="ln -s $tmp1 $name" ;;
    *) exit_fail "unknown test operation: $op" ;;
esac

log_mark=$(stat -c %s $audit_log)

# test
eval "$gen_audit_event"

# verify audit record
augrok --seek=$log_mark type==SYSCALL syscall==$op name=="$watch/" success==yes \
    || augrok --seek=$log_mark type==SYSCALL syscall==$opat name=="$watch/" \
        success==yes \
    || exit_fail "Expected record not found."

exit_pass
