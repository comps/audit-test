#!/bin/bash
###############################################################################
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
###############################################################################
#
# SFRs: FDP_ACF.1(CP), FDP_CDP.1(CP), FMT_MSA.3(CP)
#
# AUTHOR: Miroslav Vadkerti <mvadkert@redhat.com>
#
# DESCRIPTION:
# 1. Create LUKS encrypted loop device with more keys
# 2. Check if LUKS
#    + can be accessed by correct keys
#    + cannot be accessed by other keys
#    + keeps all the data consistent
#    + cannot be accessed if header reformated
#

source testcase.bash || exit 2
source tp_loop_device.bash || exit 2
source tp_luks_functions.bash || exit 2

### defaults
DMCRYPT="cryptfs"
DMCRYPTDEV="/dev/mapper/$DMCRYPT"
LUKSPASS="kc3%a9?cF]Xffd"
LUKSPASSND="2nd7k+meSs!!!"
LUKSPASSRD="meSs!!1444b_"
MOUNT="/mnt/crypt"

### functions

### main()

# be verbose
set -x

# add new loop device
create_loop_device
prepend_cleanup "remove_loop_device"

# create LUKS on loop device
create_luks $LUKSPASS

# in FIPS mode set gcrypt RNG source to /dev/urandom
if [ "x$(cat /proc/sys/crypto/fips_enabled)" = "x1" ]; then
	gcrypt_set_rng /dev/urandom
	prepend_cleanup "gcrypt_set_rng /dev/random" 
fi

# check if LUKS device uses 1 key slot
check_luks 1

# add another 2 keys
addkey_luks $LUKSPASS $LUKSPASSND
addkey_luks $LUKSPASS $LUKSPASSRD

# check if LUKS device uses 2 key slots
check_luks 3

# open LUKS Device with first pass
open_luks $DMCRYPT $LUKSPASS || exit_fail "Failed to open LUKS"

# check if kernel supports secure data flag
cryptsetup status $DMCRYPT | grep "data flag" && \
	exit_fail "Kernel doesn't support secure data flag"


# create new ext3 fs on LUKS and mount it
mkfs.ext4 $DMCRYPTDEV || exit_fail "Failed to format LUKS"
mkdir $MOUNT
prepend_cleanup "rm -rf $MOUNT"
mount -o acl $DMCRYPTDEV $MOUNT || exit_fail "Cannot mount LUKS"

# add some sample data and umount the fs
echo "CCC TEST" >> $MOUNT/testfile
chmod 644 $MOUNT/testfile
setfacl -m u:root:r $MOUNT/testfile || exit_fail "Failed to set ACL"
chcon -t etc_t $MOUNT/testfile
umount $MOUNT

# close LUKS
close_luks $DMCRYPT

# open LUKS Device with second pass
open_luks $DMCRYPT $LUKSPASSND || exit_fail "Failed to open LUKS"

# mount the test fs again
mount -o acl $DMCRYPTDEV $MOUNT || exit_fail "Cannot mount LUKS"

# check if all created data consistent
getfacl $MOUNT/testfile | tr -d '\n' | \
	egrep "user::rw-user:root:r--group::r--mask::r--other::r--" || \
	exit_fail "Failed ACL check"
ls -Z $MOUNT/testfile | egrep "etc_t" || \
	exit_fail "Failed SELinux context check"
umount $MOUNT

# close LUKS
close_luks $DMCRYPT

# open LUKS Device with bad password
open_luks $DMCRYPT "BADPASS" && exit_fail "LUKS opened with invalid password"

# reformat LUKS
create_luks $LUKSPASSRD

# open LUKS Device with correct first pass after reformat
open_luks $DMCRYPT $LUKSPASSND && exit_fail "LUKS opened with old password"

# if no failures - the test passes
exit_pass
