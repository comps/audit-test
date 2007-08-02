#!/bin/bash
###############################################################################
# (c) Copyright Hewlett-Packard Development Company, L.P., 2005
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
# Verify that mount -o context=foo can override the context on tmpfs

source tp_mount_functions.bash || exit 2

# mount tmpfs on a temporary directory location.
# collect the context of the root inode of this filesystem.
prepend_cleanup 'umount -l $mnt'
mount -t tmpfs none $mnt || exit_error "mount tmpfs failed"
context1=$(get_fsobj_context $mnt/.)
umount $mnt

# mount tmpfs using -o context= to force a context.
# collect the context of the root inode of this filesystem.
mount -t tmpfs -o context=staff_u:object_r:lspp_harness_tmpfs_t:s1:c1 \
    none $mnt || exit_error "mount tmpfs -o context failed"
context2=$(get_fsobj_context $mnt/.)
umount $mnt

# make sure context2 at least contains something that looks like a context
[[ $context2 == *:* ]] || exit_error "context2=$context2"

# the two contexts should differ; quote RHS to guard against globs
[[ $context1 != "$context2" ]] || exit_fail "both contexts are $context1"

exit_pass
