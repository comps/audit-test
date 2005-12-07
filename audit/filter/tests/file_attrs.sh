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
    # remove our files
    rm -f $file_real.hard
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
file_real=$tmp1
echo "notice: creating the test files ..."
touch $file_real 2> /dev/null || exit_error "unable to create temporary file for testing"
ln $file_real $file_real.hard || exit_error "unable to create hard linked file"

for iter_file in $file_real $file_real.hard; do
    echo ""

    # collect file information
    f_inode="$(stat -c '%i' $iter_file)"
    f_fs_mount=$(dirname $iter_file)
    while [ "$(cat /etc/fstab | awk -v DIR=$f_fs_mount '{ if ( $2 == DIR ) print $2 }')" = "" ]; do
        f_fs_mount=$(dirname $f_fs_mount)
    done
    f_fs_dev="$(cat /etc/fstab | awk -v DIR=$f_fs_mount '{ if ( $2 == DIR ) print $1 }')"
    f_fs_dev_major="$(stat -Lc '%t' $f_fs_dev)"
    f_fs_dev_minor="$(stat -Lc '%T' $f_fs_dev)"
    f_fs_dev_num=$(printf "%.2x:%.2x" "0x$f_fs_dev_major" "0x$f_fs_dev_minor")

    # display file information
    echo "notice: test file information"
    echo " path       = $iter_file"
    echo " inode      = $f_inode"
    echo " fs_mount   = $f_fs_mount"
    echo " fs_dev     = $f_fs_dev"
    echo " fs_dev_num = $f_fs_dev_num"

    ### inode check

    echo ""

    # set an audit filter
    echo "notice: setting a filter for the inode ..."
    filter_field="-F inode=$f_inode"
    auditctl -a $filter_rule $filter_field
        
    # generate an audit record
    audit_rec_gen $iter_file
        
    # look for the audit record
    echo "notice: testing for audit record ..."
    augrep --seek=$log_mark "inode==$f_inode"
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

    ### device number check
    
    echo ""

    # set an audit filter
    echo "notice: setting a filter for the device number ..."
    filter_field="-F devmajor=0x$f_fs_dev_major -F devminor=0x$f_fs_dev_minor"
    auditctl -a $filter_rule $filter_field
        
    # generate an audit record
    audit_rec_gen $iter_file
        
    # look for the audit record
    echo "notice: testing for audit record ..."
    augrep --seek=$log_mark "name==$iter_file" "dev==$f_fs_dev_num"
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

done

#
# done
#

echo ""
echo "notice: finished $(basename $0) test (exit = $ret_val)"
exit $ret_val
