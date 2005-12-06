#!/bin/sh

###############################################################################
# (c) Copyright Hewlett-Packard Development Company, L.P., 2005
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
# configuration
#

# used in auditctl (i.e., auditctl -[a|d] $filter_rule)
filter_rule="exit,always -S open"

#
# standard test harness setup
#

export PATH=/usr/local/bin:/usr/bin:/bin:/usr/sbin:/sbin
if [[ -z $TOPDIR ]]; then
    TOPDIR=$(
    while [[ ! $PWD -ef / ]]; do
        [[ -f rules.mk ]] && { echo $PWD; exit 0; }
        cd ..
    done
    exit 1
    ) || { echo "Can't find TOPDIR, where is rules.mk?" >&2; exit 2; }
    export TOPDIR
fi
PATH=$TOPDIR/utils:$PATH

source functions.bash

#
# helper functions
#

# override the test harness cleanup function
prepend_cleanup '
    # remove the filter we set earlier
    auditctl -d $filter_rule $filter_field 2>/dev/null'

# generate a successful audit record for the given file and update the audit
# log marker
function audit_rec_gen_ok {
    if [ -f "$1" ]; then
        log_mark=$(stat -c %s $audit_log)
        cat "$1" > /dev/null
    else
        exit_error "unable to find file \"$1\""
    fi
}

# generate a failure audit record for the given file and update the audit log
# marker
function audit_rec_gen_fail {
    [ "$TEST_USER" = "" ] && exit_error "run in the harness or define \$TEST_USER"
    if [ -f "$1" ]; then
        log_mark=$(stat -c %s $audit_log)
        /bin/su $TEST_USER bash -c "cat \"$1\"" 2> /dev/null
    else
        exit_error "unable to find file \"$1\""
    fi
}


#
# main
#

# startup banner
echo "notice: starting $(basename $0) test ($(date))"
echo ""

# return value
ret_val=0

# audit log marker
log_mark=$(stat -c %s $audit_log)

# create the test files
echo "notice: creating the test file ..."
touch $tmp1 2> /dev/null || exit_error "unable to create temporary file for testing"
chmod 0600 $tmp1 || exit_error "unable to set the permissions on the test file"
chown root:root $tmp1 || exit_error "unable to set the permissions on the test file"

# get syscall information
syscall_name="open"
syscall_num="$(grep __NR_${syscall_name} /usr/include/asm/unistd.h | awk '{ print $3}')"

# display syscall information
echo "notice: syscall information"
echo " platform       = $(uname -i)"
echo " syscall name   = $syscall_name"
echo " syscall number = $syscall_num"

### success check

echo ""

# set an audit filter
echo "notice: setting a filter for the syscall success ..."
filter_field="-F success=1"
auditctl -a $filter_rule $filter_field

# generate an audit event
audit_rec_gen_ok $tmp1

# check for the audit record
echo "notice: testing for audit record ..."
augrep --seek=$log_mark "syscall==$syscall_num" "name==$tmp1" "success==yes"
ret_val_tmp=$?
[ "$ret_val" = "0" ] && ret_val=$ret_val_tmp

# display the result
if [ "$ret_val_tmp" = "0" ]; then
    echo "notice: found audit record - PASS"
else
    echo "notice: did not find audit record - FAIL"
fi

# remove the filter
auditctl -d $filter_rule $filter_field
filter_field=""

### fail check

echo ""

# set an audit filter
echo "notice: setting a filter for the syscall failure ..."
filter_field="-F success!=1"
auditctl -a $filter_rule $filter_field

# generate an audit event
audit_rec_gen_fail $tmp1

# check for the audit record
echo "notice: testing for audit record ..."
augrep --seek=$log_mark "name==$tmp1"
ret_val_tmp=$?
[ "$ret_val" = "0" ] && ret_val=$ret_val_tmp

# display the result
if [ "$ret_val_tmp" = "0" ]; then
    echo "notice: found audit record - PASS"
else
    echo "notice: did not find audit record - FAIL"
fi

# remove the filter
auditctl -d $filter_rule $filter_field
filter_field=""

### syscall name check

echo ""

# set an audit filter
echo "notice: setting a filter for the syscall name ..."
filter_rule="exit,always -S open"
auditctl -a $filter_rule

# generate an audit event
audit_rec_gen_ok $tmp1

# check for the audit record
echo "notice: testing for audit record ..."
augrep --seek=$log_mark "name==$tmp1" "syscall==$syscall_num"
ret_val_tmp=$?
[ "$ret_val" = "0" ] && ret_val=$ret_val_tmp

# display the result
if [ "$ret_val_tmp" = "0" ]; then
    echo "notice: found audit record - PASS"
else
    echo "notice: did not find audit record - FAIL"
fi

# remove the filter
auditctl -d $filter_rule

### syscall number check

echo ""

# set an audit filter
echo "notice: setting a filter for the syscall number ..."
filter_rule="exit,always -S $syscall_num"
auditctl -a $filter_rule

# generate an audit event
audit_rec_gen_ok $tmp1

# check for the audit record
echo "notice: testing for audit record ..."
augrep --seek=$log_mark "name==$tmp1" "syscall==$syscall_num"
ret_val_tmp=$?
[ "$ret_val" = "0" ] && ret_val=$ret_val_tmp

# display the result
if [ "$ret_val_tmp" = "0" ]; then
    echo "notice: found audit record - PASS"
else
    echo "notice: did not find audit record - FAIL"
fi

# remove the filter
auditctl -d $filter_rule

#
# done
#

echo ""
echo "notice: finished $(basename $0) test (exit = $ret_val)"
exit $ret_val
