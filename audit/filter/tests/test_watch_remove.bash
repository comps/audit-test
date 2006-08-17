#!/bin/bash
###############################################################################
# (c) Copyright Hewlett-Packard Development Company, L.P., 2006
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
# PURPOSE: Test the ability to filter based on a filesystem watch,
#          specifically for objects removed at the watched path.

source filter_functions.bash || exit 2

# setup
op=$1

tmpd=$(mktemp -d) || exit_fail "create tempdir failed"
name="$tmpd/foo"
case $op in
    unlink) touch $name
            gen_audit_event="rm $name" ;;
    rmdir)  mkdir $name
            gen_audit_event="rmdir $name" ;;
    *) exit_fail "unknown test operation" ;;
esac

auditctl -a exit,always -S $op -F path=$name

prepend_cleanup "
    auditctl -d exit,always -S $op -F path=$name
    rm -rf $tmpd"

log_mark=$(stat -c %s $audit_log)

# test
eval "$gen_audit_event"

# verify audit record
augrok --seek=$log_mark "type=~SYSCALL" name_1==$name success==yes \
    || exit_fail "Expected record not found."

exit_pass
