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
# Test the ability to filter for objects created at a watched path.

source filter_functions.bash || exit 2

# setup
op=$1

tmpd=$(mktemp -d) || exit_fail "create tempdir failed"
name="$tmpd/foo"

auditctl -a exit,always -S $op -F path=$name

prepend_cleanup "
    auditctl -d exit,always -S $op -F path=$name
    rm -rf $tmpd"

case $op in
    link)    gen_audit_event="ln $tmp1 $name"
             filter_fields="name#a==$tmp1 name#b==$name" ;;
    mkdir)   gen_audit_event="mkdir $name"
             filter_fields="name==$name" ;;
    open)    gen_audit_event="touch $name"
             filter_fields="name==$name" ;;
    rename)  gen_audit_event="mv $tmp1 $name"
             filter_fields="name#a==$tmp1 name#b==$name" ;;
    symlink) gen_audit_event="ln -s $tmp1 $name"
             filter_fields="name#a==$tmp1 name#b==$name" ;;
    *) exit_fail "unknown test operation" ;;
esac

log_mark=$(stat -c %s $audit_log)

# test
eval "$gen_audit_event"

# verify audit record
augrok --seek=$log_mark type==SYSCALL syscall==$op $filter_fields success==yes \
    || exit_fail "Expected record not found."

exit_pass
