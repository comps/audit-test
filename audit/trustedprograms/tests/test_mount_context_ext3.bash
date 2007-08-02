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
# Verify that mount -o context=foo can overide the context on ext3

source tp_mount_functions.bash || exit 2

# create a loopback ext3 filesystem.
# separate losetup from mount so that context= only applies to the mount.
dd if=/dev/zero of=$tmp1 count=0 bs=1 seek=100M || exit_error "dd failed"
mkfs.ext3 -F $tmp1 || exit_error "mkfs.ext3 failed"
unset loop
prepend_cleanup 'losetup -d $loop'
loop=$(losetup -f) || exit_error "losetup -f failed"
losetup $loop $tmp1

# mount ext3fs on a temporary directory location.
# collect the context of the root inode of this filesystem.
prepend_cleanup 'umount -l $mnt'
mount $loop $mnt || exit_error "mount $loop failed"
context1=$(get_fsobj_context $mnt/.)
umount $mnt

# mount ext2fs using -o context= to force a context.
# collect the context of the root inode of this filesystem.
mount -o context=staff_u:object_r:lspp_harness_tmpfs_t:s1:c1 \
    $loop $mnt || exit_error "mount $loop failed"
context2=$(get_fsobj_context $mnt/.)
umount $mnt

# make sure context2 at least contains something that looks like a context
[[ $context2 == *:* ]] || exit_error "context2=$context2"

# the two contexts should differ; quote RHS to guard against globs
[[ $context1 != "$context2" ]] || exit_fail "both contexts are $context1"

exit_pass
