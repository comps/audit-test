#!/bin/bash
###############################################################################
#   Copyright (c) 2014 Red Hat, Inc. All rights reserved.
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
# AUTHOR: Jiri Jaburek <jjaburek@redhat.com>
#

source testcase.bash || exit 2
set -x

# args: <fstype> <op> [op] ...
#  ie.: ext4 ro noexec nosuid
# (the operations are not split because of the overhead of setting up an image)

#
# this test tests various fs-independent mount options (ro,noexec,..), whether
# they work as expected
#

fstype="$1"
shift

# setup
mntpoint=$(mktemp -d) || exit_error  # absolute path required
prepend_cleanup "rmdir \"$mntpoint\""

mntimage=$(mktemp) || exit_error
prepend_cleanup "rm -f \"$mntimage\""

# 16MB minimum for xfs
dd if=/dev/zero of="$mntimage" bs=16M count=1 || exit_error

mntdev=$(losetup --show -f "$mntimage") || exit_error
prepend_cleanup "losetup -d \"$mntdev\""

mkfs -t "$fstype" "$mntdev" || exit_error
mount -o loop "$mntdev" "$mntpoint" || exit_error
prepend_cleanup "umount \"$mntpoint\""

# test
while [ $# -gt 0 ]; do
    op="$1"
    shift
    case "$op" in
        ro)
            touch "$mntpoint"/ro.x || \
                exit_error "ro: could not create file on rw filesystem"
            mount -o remount,ro "$mntpoint"
            touch "$mntpoint"/ro.y && \
                exit_fail "ro: could create file on ro filesystem"
            mount -o remount,rw "$mntpoint"
            ;;
        noexec)
            cp /bin/true "$mntpoint"/noexec.true || exit_error
            "$mntpoint"/noexec.true || \
                exit_error "noexec: could not exec binary on exec filesystem"
            mount -o remount,noexec "$mntpoint"
            "$mntpoint"/noexec.true && \
                exit_fail "noexec: could exec binary on noexec filesystem"
            mount -o remount,exec "$mntpoint"
            ;;
        nosuid)
            cp /bin/whoami "$mntpoint"/nosuid.whoami || exit_error
            chown nobody:nobody "$mntpoint"/nosuid.whoami || exit_error
            chmod u+s "$mntpoint"/nosuid.whoami || exit_error
            [ "$(whoami)" != "$("$mntpoint"/nosuid.whoami)" ] || \
                exit_error "nosuid: suid bit doesn't work on suid filesystem"
            mount -o remount,nosuid "$mntpoint"
            [ "$(whoami)" != "$("$mntpoint"/nosuid.whoami)" ] && \
                exit_fail "nosuid: suid bit works on nosuid filesystem"
            mount -o remount,suid "$mntpoint"
            ;;
        nodev)
            mknod "$mntpoint"/nodev.null c 1 3 || exit_error
            cat "$mntpoint"/nodev.null || \
                exit_error "nodev: device file cannot be read on dev filesystem"
            mount -o remount,nodev "$mntpoint"
            cat "$mntpoint"/nodev.null && \
                exit_fail "nodev: device file can be read on nodev filesystem"
            mount -o remount,dev "$mntpoint"
            ;;
    esac
done

exit_pass

# vim: sts=4 sw=4 et :
