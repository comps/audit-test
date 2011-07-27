#!/bin/bash
# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
#
#   Copyright (c) 2011 Red Hat, Inc. All rights reserved.
#
#   This copyrighted material is made available to anyone wishing
#   to use, modify, copy, or redistribute it subject to the terms
#   and conditions of the GNU General Public License version 2.
#
#   This program is distributed in the hope that it will be
#   useful, but WITHOUT ANY WARRANTY; without even the implied
#   warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
#   PURPOSE. See the GNU General Public License for more details.
#
#   You should have received a copy of the GNU General Public
#   License along with this program; if not, write to the Free
#   Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
#   Boston, MA 02110-1301, USA.
#
# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
#
# SFR: FDP_ITC.2(LS)
#
# Author: Milos Malik <mmalik@redhat.com>
#
# Description:
# The test checks if
# * filesystem with security attributes can be mounted
# * labels of files on such filesystem can be changed
# * CAP_SYS_ADMIN capability is needed
# * mlsfileupgrade attribute is needed
# * mount point and files are labeled correctly after mount operation

source tp_mount_functions.bash || exit 2
source tp_context_functions.bash || exit 2

# create a loopback ext3 filesystem
dd if=/dev/zero of=$tmp1 count=100k bs=1024 || exit_error "dd failed"
mkfs.ext3 -F $tmp1 || exit_error "mkfs.ext3 failed"

unset loop
prepend_cleanup 'losetup -d $loop'
loop=$(losetup -f) || exit_error "losetup -f failed"
losetup $loop $tmp1

# it is not possible to mount the filesystem without CAP_SYS_ADMIN capability
capsh --drop=cap_sys_admin -- -c "mount -o defcontext='system_u:object_r:lspp_test_file_t:s0',fscontext='system_u:object_r:lspp_test_file_t:s0' $loop $mnt"
context1=$(get_fsobj_context $mnt/.)
umount $mnt

# lspp_harness_t should have mlsfileupgrade attribute
seinfo -amlsfileupgrade -x | grep lspp_harness_t || exit_error "lspp_harness_t does NOT contain mlsfileupgrade attribute"

# lspp_test_generic_t should NOT have mlsfileupgrade attribute
seinfo -amlsfileupgrade -x | grep lspp_test_generic_t && exit_error "lspp_test_generic_t contains mlsfileupgrade attribute"

# create some files on the ext3fs for next 2 test cases
prepend_cleanup 'umount -l $mnt'
mount -o defcontext="system_u:object_r:lspp_test_file_t:s0",fscontext="system_u:object_r:lspp_test_file_t:s0" $loop $mnt || exit_error "mount $loop failed"
for I in s0:c0 s1:c1 s2:c2 s3:c3 ; do
    echo "content-$I" > $mnt/file-$I
    chcon -l $I $mnt/file-$I
done
umount $mnt

losetup -d $loop
SCORE=0
cp $tmp1 $tmp2
loop=$(losetup -f) || exit_error "losetup -f failed"
losetup $loop $tmp1

# mount ext3fs with defcontext and fscontext options
mount -o defcontext="system_u:object_r:lspp_test_file_t:s0",fscontext="system_u:object_r:lspp_test_file_t:s0" $loop $mnt || exit_error "mount $loop failed"
context2=$(get_fsobj_context $mnt/.)
for I in s0:c0 s1:c1 s2:c2 s3:c3 ; do
    runcon -t lspp_harness_t chcon -l s4:c4 $mnt/file-$I || exit_fail "chcon should succeed"
    runcon -t lspp_test_generic_t chcon -l $I $mnt/file-$I && exit_fail "chcon should fail"
done
umount $mnt

losetup -d $loop
loop=$(losetup -f) || exit_error "losetup -f failed"
losetup $loop $tmp2

# mount ext3fs with context option
mount -o context="system_u:object_r:lspp_test_file_t:s0" $loop $mnt || exit_error "mount $loop failed"
context3=$(get_fsobj_context $mnt/.)
for I in s0:c0 s1:c1 s2:c2 s3:c3 ; do
    runcon -t lspp_harness_t chcon -l s4:c4 $mnt/file-$I && exit_fail "chcon should fail"
    runcon -t lspp_test_generic_t chcon -l $I $mnt/file-$I && exit_fail "chcon should fail"
done
umount $mnt

losetup -d $loop

# make sure that context1 does NOT contain lspp_test_file_t
[[ $context1 != *:lspp_test_file_t:* ]] || exit_fail "context1=$context1 is not expected"

# make sure that context2 contains lspp_test_file_t
[[ $context2 == *:lspp_test_file_t:* ]] || exit_fail "context2=$context2 is not expected"

# make sure that context3 contains lspp_test_file_t
[[ $context3 == *:lspp_test_file_t:* ]] || exit_fail "context3=$context3 is not expected"

exit_pass
