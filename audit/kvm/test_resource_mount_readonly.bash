#!/usr/bin/env bash
#
#   Copyright 2010, 2011 International Business Machines Corp.
#   Copyright 2010, 2011 Ramon de Carvalho Valle
#
#   This program is free software: you can redistribute it and/or modify
#   it under the terms of the GNU General Public License as published by
#   the Free Software Foundation, either version 2 of the License, or
#   (at your option) any later version.
#
#   This program is distributed in the hope that it will be useful,
#   but WITHOUT ANY WARRANTY; without even the implied warranty of
#   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#   GNU General Public License for more details.
#
#   You should have received a copy of the GNU General Public License
#   along with this program.  If not, see <http://www.gnu.org/licenses/>.
#

#   test_resource_mount_readonly.bash
#
#   Assert resources associated with the processes representing virtual
#   machine environments can not be modified when accessed in read-only mode.


source testcase.bash || exit 2

set -x

LOOPDEV=$(losetup -f)
append_cleanup "umount /mnt"
append_cleanup "kpartx -d $LOOPDEV"
append_cleanup "losetup -d $LOOPDEV"

for i in $(seq $first $last); do
	umount /mnt
	kpartx -d $LOOPDEV
	losetup -d $LOOPDEV

	eval "losetup $LOOPDEV \$kvm_guest_${i}_resource"
	kpartx -a $LOOPDEV
	mount -o ro /dev/mapper/loop0p1 /mnt

	touch /mnt/testfile

	if [[ $? -eq 0 ]]; then
		exit_fail
	fi

	umount /mnt
	kpartx -d $LOOPDEV
	losetup -d $LOOPDEV
done

exit_pass

#   vim: set noet sw=8 ts=8 tw=0:
