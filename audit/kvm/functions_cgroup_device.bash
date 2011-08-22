#!/bin/bash
################################################################################
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
# AUTHOR: Eduard Benes <ebenes@redhat.com>
# DESCRIPTION: Helper functions for cgroup device tests for SRF FDP_ACF.1(VIRT)
#


source testcase.bash || exit 2

#
# Global variables
#

# Do not change this or make sure to update also domain template XML files.
dom1="guest1"
img_path="/var/lib/libvirt/images"

# Note: Only block devices capable of acting as disk storage are supported.
cgroup_block_device="/dev/ram0"
cgroup_block_device_major_minor="`/usr/bin/stat -c %t:%T $cgroup_block_device`"
cgroup_block_device_list_entry="b $cgroup_block_device_major_minor rwm"

cgroup_char_device="/dev/null"
cgroup_char_device_major_minor="`/usr/bin/stat -c %t:%T $cgroup_char_device`"
cgroup_char_device_list_entry="c $cgroup_char_device_major_minor rwm"

mnt_test_point="/mnt/block_device_test"


#
# General helper functions
#

generate_block_dev_file() {
    /bin/cat > block_dev.xml << EOX
    <disk type='block' device='disk'>
      <driver name='qemu' type='raw'/>
      <source dev='$cgroup_block_device'/>
      <target dev='hda' bus='ide'/>
      <address type='drive' controller='0' bus='0' unit='0'/>
    </disk>
EOX
    append_cleanup "/bin/rm -f block_dev.xml"
}

# This is harcoded for XML templates due to static labeling.
prepare_guest_domain() {
    # Create empty fake disk images
    /bin/dd if=/dev/zero of=$img_path/${dom1}.img bs=1M count=1
    # Set preconfigured disk image labels for static labeling a.k.a MLS ready
    /usr/bin/chcon system_u:object_r:svirt_image_t:s0:c50,c70 $img_path/${dom1}.img

    append_cleanup "/bin/rm -f $img_path/${dom1}.img"
}

# Create and start a guest domain.
create_guest_domain() {
    /bin/sed '/CGROUP_DEV_CONFIG/ {r block_dev.xml
    d}' ${1}-template.xml > ${1}.xml

    append_cleanup "/bin/rm -f ${1}.xml"
    prepend_cleanup "/usr/bin/virsh destroy $1"

    /usr/bin/virsh create ${1}.xml && sleep 3
    return $?
}

destroy_guest_domain() {
    /usr/bin/virsh destroy $1 || exit_error "Failed to destroy domain"
    sleep 2
}

get_guest_domain_pid() {
    local pid
    for pid in `pgrep qemu-kvm` ; do
        /bin/ps -fp $pid | /bin/grep $1 > /dev/null && echo $pid && break
    done
}


#
# Checks for any cgroup testing prerequities. Return 0 only on success.
#

check_installed_packages() {
    /bin/rpm -q $@
    return $?
}

check_cgroup_hierarchy() {
    [ -d "/cgroup/cpu/libvirt/qemu/$1" ] && \
    [ -d "/cgroup/devices/libvirt/qemu/$1" ] && \
    [ -d "/cgroup/cpuacct/libvirt/qemu/$1" ] && \
    [ -d "/cgroup/memory/libvirt/qemu/$1" ] && \
    [ -d "/cgroup/blkio/libvirt/qemu/$1" ] && \
    [ -d "/cgroup/freezer/libvirt/qemu/$1" ] && \
    [ -d "/cgroup/cpuset/libvirt/qemu/$1" ]
    return $?
}

check_cgroups_availability() {
    /sbin/service cgconfig status && \
    /bin/cat /proc/cgroups && \
    check_cgroup_hierarchy ""
    return $?
}


#
# Checks for required object and subject security attributes
#

# check_cgroup_tasks
check_cgroup_tasks() {
    local cgroup_dir="/cgroup/devices/libvirt/qemu/$2"
    /bin/grep "$1" $cgroup_dir/tasks
    return $?
}

check_cgroup_procs() {
    local cgroup_dir="/cgroup/devices/libvirt/qemu/$2"
    /bin/grep "$1" $cgroup_dir/cgroup.procs
    return $?
}

check_cgroup_devices_list() {
    local cgroup_dir="/cgroup/devices/libvirt/qemu/$1"
    /bin/grep "$2" $cgroup_dir/devices.list
    return $?
}

add_to_devices_file() {
    local cgroup_dir="/cgroup/devices/libvirt/qemu/$1"
    echo "$2" > $cgroup_dir/devices.$3
    return $?
}

# cmd proc# where#
add_proc_to_tasks() {
    local cgroup_dir="/cgroup/devices/libvirt/qemu/$2"
    echo $1 > $cgroup_dir/tasks
    return $?
}


#
# Other common functions
#

start_guest() {
    create_guest_domain $dom1 || exit_fail "creating domain"

    check_cgroup_hierarchy $dom1 || exit_fail "cgroup hierarchy"
    dom_pid=`get_guest_domain_pid $1`
    check_cgroup_tasks $dom_pid $dom1 || exit_fail "cgroup.tasks"
    check_cgroup_procs $dom_pid $dom1 || exit_fail "cgroup.procs"
    # We have only RW permissions after creation of a guest domain,
    # therefore we don't use $cgroup_*_device_list_entry gloval variable here.
    check_cgroup_devices_list $dom1 "b $cgroup_block_device_major_minor rw" || \
        exit_fail "block devices.list"
    check_cgroup_devices_list $dom1 "c $cgroup_char_device_major_minor rw" || \
        exit_fail "char devices.list"
}

create_devices_test_subgroup() {
    /bin/mkdir /cgroup/devices/libvirt/qemu/$1
    sleep 1
    [ -e "/cgroup/devices/libvirt/qemu/$1/cgroup.procs" ] && \
    [ -e "/cgroup/devices/libvirt/qemu/$1/devices.allow" ] && \
    [ -e "/cgroup/devices/libvirt/qemu/$1/devices.deny" ] && \
    [ -e "/cgroup/devices/libvirt/qemu/$1/devices.list" ] && \
    [ -e "/cgroup/devices/libvirt/qemu/$1/notify_on_release" ] && \
    [ -e "/cgroup/devices/libvirt/qemu/$1/tasks" ]
    return $?
}

prepare_block_device() {
    # Zero out the space you need on ramdisk device, e.g. 16M
    /bin/dd if=/dev/zero of=$cgroup_block_device bs=1K count=16K
    # Make filesystem on the ram disk
    /sbin/mkfs.ext4 $cgroup_block_device

    append_cleanup restorecon -RvvF $cgroup_block_device
    /usr/bin/chcon system_u:object_r:svirt_image_t:s0:c50,c70 $cgroup_block_device
}

get_my_pid() {
    echo $$
}

# Block device functionality testing function

rw_block_device_test() {
    # Expects a mounted device already on mnt_test_point
    date > `/bin/mktemp -p $mnt_test_point rwtest.XXXX`
    return $?
}

open_block_device_test() {
    append_cleanup /bin/umount -lf $mnt_test_point
    append_cleanup /bin/rm -rf $mnt_test_point

    # Mount the ram disk
    /bin/mkdir -p $mnt_test_point
    /bin/mount $cgroup_block_device $mnt_test_point

    return $?
}

mknod_test() {
    append_cleanup /bin/rm -rf $mnt_test_point/char_dev
    append_cleanup /bin/rm -rf $mnt_test_point/block_dev

    ref_c_stat="`/usr/bin/stat -c '%t %T' $cgroup_char_device`"
    /bin/mknod $mnt_test_point/char_dev c $ref_c_stat # most likely: /dev/null
    c_stat="`/usr/bin/stat -c '%t %T' $mnt_test_point/char_dev`"

    ref_b_stat="`/usr/bin/stat -c '%t %T' $cgroup_block_device`"
    /bin/mknod $mnt_test_point/block_dev b $ref_b_stat # most likely: /dev/ram0
    b_stat="`/usr/bin/stat -c '%t %T' $mnt_test_point/block_dev`"

    [ "$ref_b_stat" == "$b_stat" ] && [ "$ref_c_stat" == "$c_stat" ]
    return $?
}

# Character device tests

char_device_test() {
    date > /dev/null
    return $?
}
