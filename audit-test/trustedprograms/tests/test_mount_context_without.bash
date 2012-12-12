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
# SFR: FDP_ITC.1(LS)
#
# Author: Milos Malik <mmalik@redhat.com>
#
# Description:
# The test checks if
# * filesystem without security attributes can be mounted
# * labels of files on such filesystem can be changed
# * CAP_SYS_ADMIN capability is needed
# * mount point and files are labeled correctly after mount operation
#
# Note:
# The test needs to have mcstransd service stopped during the testing
#

source tp_mount_functions.bash || exit 2
source tp_context_functions.bash || exit 2

# stop mcstransd service (the mcstransd is by default started during testing)
stop_service mcstransd
prepend_cleanup 'start_service mcstransd'

# create a loopback VFAT filesystem
dd if=/dev/zero of=$tmp1 count=100k bs=1024 || exit_error "dd failed"
unset loop
prepend_cleanup 'losetup -d $loop'
loop=$(losetup -f) || exit_error "losetup -f failed"
losetup $loop $tmp1
mkfs.vfat $loop || exit_error "mkfs.vfat failed"

# it is not possible to mount the filesystem without CAP_SYS_ADMIN capability$
capsh --drop=cap_sys_admin -- -c "mount -o fscontext='system_u:object_r:lspp_test_file_t:s0' $loop $mnt"
context1=$(get_fsobj_context $mnt/.)
umount $mnt

# create some files on the VFAT filesystem for next 2 test cases
prepend_cleanup 'umount -l $mnt'
mount -o fscontext="system_u:object_r:lspp_test_file_t:s0" $loop $mnt || exit_error "mount $loop failed"
for I in s0 s1 s2 s3 ; do
    echo "content-$I" > $mnt/file-$I
done
umount $mnt

losetup -d $loop
cp $tmp1 $tmp2
loop=$(losetup -f) || exit_error "losetup -f failed"
losetup $loop $tmp1

# mount VFAT fs with fscontext option
mount -o fscontext="system_u:object_r:lspp_test_file_t:s1" $loop $mnt || exit_error "mount $loop failed"
context2=$(get_fsobj_context $mnt/.)
for I in s0 s2 s3 ; do
    runcon -t lspp_harness_t chcon -l s4 $mnt/file-$I && exit_fail "chcon should fail"
    runcon -t lspp_test_generic_t chcon -l $I $mnt/file-$I && exit_fail "chcon should fail"
done
umount $mnt

losetup -d $loop
loop=$(losetup -f) || exit_error "losetup -f failed"
losetup $loop $tmp2

# mount VFAT fs with context option
mount -o context="system_u:object_r:lspp_test_file_t:s2" $loop $mnt || exit_error "mount $loop failed"
context3=$(get_fsobj_context $mnt/.)
for I in s0 s1 s3 ; do
    runcon -t lspp_harness_t chcon -l s4 $mnt/file-$I && exit_fail "chcon should fail"
    runcon -t lspp_test_generic_t chcon -l $I $mnt/file-$I && exit_fail "chcon should fail"
done
umount $mnt
#
#losetup -d $loop

# make sure that context1 does NOT contain lspp_test_file_t
[[ $context1 != *:lspp_test_file_t:s0 ]] || exit_fail "context1=$context1 is not expected"

# make sure that context2 contains lspp_test_file_t
[[ $context2 == *:lspp_test_file_t:s1 ]] || exit_fail "context2=$context2 is not expected"

# make sure that context3 contains lspp_test_file_t
[[ $context3 == *:lspp_test_file_t:s2 ]] || exit_fail "context3=$context3 is not expected"

exit_pass
