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
# process_attrs.sh - This filter test is designed to test the ability to use
#                    auditctl to specify a kernel filter for the audit logs.
#                    Specifically it tests:
#           Test 1 - The ability to filter based on login uid (auid)

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
    [ -n "$filter_field" ] && auditctl -d $filter_rule $filter_field 2>/dev/null'

# generate an audit record for the given file and update the
# audit log marker
function audit_rec_gen {
    if [ -f "$1" ]; then
        log_mark=$(stat -c %s $audit_log)
        cat "$1" > /dev/null
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

# get user information
user_auid="$(cat /proc/self/loginuid)"

# display file information
echo "notice: user information"
echo " uid       = $(id -u)"
echo " login id  = $user_auid"

### Test 1 - Filter on login uid (auid)

# set an audit filter
echo "notice: setting a filter for the login uid ..."
filter_field="-F auid=$user_auid"
auditctl -a $filter_rule $filter_field

# generate an audit event
audit_rec_gen $tmp1

# check for the audit record
echo "notice: testing for audit record ..."
augrok --seek=$log_mark "name==$tmp1" "auid==$user_auid"
ret_val=$?

#
# done
#

echo ""
echo "notice: finished $(basename $0) test (exit = $ret_val)"
exit $ret_val
