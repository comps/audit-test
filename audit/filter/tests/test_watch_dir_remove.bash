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
# Test the ability to filter for objects removed from a watched directory.

source filter_functions.bash || exit 2

# setup
op=$1

tmpd=$(mktemp -d) || exit_fail "create tempdir failed"
watch="$tmpd"
name="$tmpd/foo"

auditctl -a exit,always -S $op -F path=$watch

prepend_cleanup "
    auditctl -d exit,always -S $op -F path=$watch
    rm -rf $tmpd"

case $op in
    rename) touch $name
            gen_audit_event="mv $tmp1 $name" ;;
    rmdir)  mkdir $name
            gen_audit_event="rmdir $name" ;;
    unlink) touch $name
            gen_audit_event="rm $name" ;;
    *) exit_fail "unknown test operation: $op" ;;
esac

log_mark=$(stat -c %s $audit_log)

# test
eval "$gen_audit_event"

# verify audit record
augrok --seek=$log_mark type==SYSCALL syscall==$op name=="$watch/" success==yes \
    || exit_fail "Expected record not found."

exit_pass
