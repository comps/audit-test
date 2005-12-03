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

# used in auditctl (i.e., auditctl -[a|d] $filter_str)
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
    [ -n "$filter_field" ] && auditctl -d $filter_rule -F $filter_field 2>/dev/null'

# generate an audit record for the given file and update the
# audit log marker
function audit_rec_gen {
    if [ -f "$1" ]; then
        log_mark=$(stat -c %s $audit_log)
        cat $tmp1 > /dev/null
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

# create the test file
echo "notice: creating a temporary file ..."
touch $tmp1 2> /dev/null || exit_error "unable to create temporary file for testing"

# collect file information
f_inode="$(stat -c '%i' $tmp1)"
echo "notice: temporary file information"
echo " path      = $tmp1"
echo " inode     = $f_inode"

for iter in "inode=$f_inode"; do
    # set an audit filter
    echo "notice: setting a filter for $iter ..."
    filter_field="$iter"
    auditctl -a $filter_rule -F $filter_field

    # generate an audit record
    audit_rec_gen $tmp1

    # look for the audit record
    echo "notice: testing for audit record ..."
    augrep --seek=$log_mark ${filter_field/=/==}
    ret_val_tmp=$?
    [ "$ret_val" = "0" ] && ret_val=$ret_val_tmp

    # remove the filter
    auditctl -d $filter_rule -F $filter_field
    filter_field=""
done

#
# done
#

echo ""
echo "notice: finished $(basename $0) test (exit = $ret_val)"
exit $ret_val
