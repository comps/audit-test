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
# file_attrs.sh - This filter test is designed to test the ability to use
#                 auditctl to specify a kernel filter for the audit logs.
#                 Specifically it tests:
#        Test 1 - The ability to filter based on an inode number
#        Test 2 - The ability to filter based on a device number

#
# configuration
#

source filter_functions.bash

# used in auditctl (i.e., auditctl -[a|d] $filter_rule)
filter_rule="exit,always -S open"

#
# helper functions
#

# override the test harness cleanup function
prepend_cleanup '
    # remove our files
    rm -f $file_real.hard
    # remove the filter we set earlier
    [ -n "$filter_field" ] && auditctl -d $filter_rule $filter_field 2>/dev/null'

#
# main
#

# return value
ret_val=0

# create the test files
file_real=$tmp1
ln $file_real $file_real.hard || exit_error "unable to create hard linked file"

for iter_file in $file_real $file_real.hard; do

    # collect file information
    f_inode="$(stat -c '%i' $iter_file)"
    f_fs_mount=$(dirname $iter_file)
    while [ "$(mount | awk -v DIR=$f_fs_mount '{ if ( $3 == DIR ) print $3 }')" = "" ]; do
        f_fs_mount=$(dirname $f_fs_mount)
    done
    f_fs_dev="$(mount | awk -v DIR=$f_fs_mount '{ if ( $3 == DIR ) print $1 }')"
    f_fs_dev_major="$(stat -Lc '%t' $f_fs_dev)"
    f_fs_dev_minor="$(stat -Lc '%T' $f_fs_dev)"
    f_fs_dev_num=$(printf "%.2x:%.2x" "0x$f_fs_dev_major" "0x$f_fs_dev_minor")

    ### Test 1 - Filter on an inode number

    # set an audit filter
    filter_field="-F inode=$f_inode"
    auditctl -a $filter_rule $filter_field

    # audit log marker
    log_mark=$(stat -c %s $audit_log)
        
    # generate an audit record
    do_open_file $iter_file
        
    # look for the audit record
    augrok --seek=$log_mark "inode==$f_inode"
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

    
    ### Test 2 - Filter on a device number

    # set an audit filter
    filter_field="-F devmajor=0x$f_fs_dev_major -F devminor=0x$f_fs_dev_minor"
    auditctl -a $filter_rule $filter_field

    # audit log marker
    log_mark=$(stat -c %s $audit_log)
        
    # generate an audit record
    do_open_file $iter_file
        
    # look for the audit record
    augrok --seek=$log_mark "name==$iter_file" "dev==$f_fs_dev_num"
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

exit $ret_val
