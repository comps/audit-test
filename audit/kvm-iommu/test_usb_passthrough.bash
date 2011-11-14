#!/bin/bash -x
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
# AUTHOR: Miroslav Vadkerti <mvadkert@redhat.com>
# DESCRIPTION: Tests for SRF FDP_ACF.1(VIRT) - USB passthrough
#
# These tests are intended to test USB passthrough. Please see
# the functions comments for more info about the testing
#

source testcase.bash || exit 2
source usb_device.conf || exit 2

#
# Global variables
#

# audit log
AUDIT_LOG="/var/log/audit/audit.log"

# default usb context
USB_DEFCON="usb_device_t"

# Test scenario to test for by calling corresponding function
scenario=$1

# List of some virt specific packages required for our testing
pkg_list="qemu-kvm libvirt libvirt-client"

# check if usb_device_id specified
[[ $usb_device_id == *X* ]] && \
    exit_error "USB device in usb_device.conf not specified"

# USB device variables
usb_vendor="$(echo $usb_device_id | cut -d: -f1)"
[ "x" = "x$usb_vendor" ] && \
    exit_error "USB device configuration is invalid - check usb_device.conf"
usb_product="$(echo $usb_device_id | cut -d: -f2)"
[ "x" = "x$usb_product" ] && \
    exit_error "USB device configuration is invalid - check usb_device.conf"
# Please note if the USB device has more endpoints only the first is used
# for testing
usb_bus=$(lsusb | grep -m1 $usb_device_id | tr ':' ' ' | cut -d\  -f2)
[ "x" = "x$usb_bus" ] && \
    exit_error "USB device $usb_device_id not found"
usb_device=$(lsusb | grep -m1 $usb_device_id | tr ':' ' ' | cut -d\  -f4)
[ "x" = "x$usb_bus" ] && \
    exit_error "USB device $usb_device_id not found"

# Do not change this or make sure to update also domain template XML files.
dom1="guest1"
dom2="guest1-dynamic"
dom3="guest2-dynamic"
img_path="/var/lib/libvirt/images"

#
# General helper functions
#

function get_audit_mark {
    echo "$(stat -c %s $AUDIT_LOG)"
}

relabel_usb_device_files_for_domain() {
    # Using context label from guest domain XML template file will allow us
    # to relabel device files also for guests which are not running.
    local category=`grep "<label>" $1-template.xml | \
        sed "s/.*:\(c[0-9]*,c[0-9]*\).*/\1/"`
    /usr/bin/chcon -v system_u:object_r:svirt_image_t:s0:$category \
        /dev/bus/usb/$usb_bus/$usb_device
    append_cleanup  /sbin/restorecon -RvvvF \
        /dev/bus/usb/$usb_bus/$usb_device
}

reload_kvm_module_for_unsafe_interrupts() {
    # There should be no running guest domain or we will fail
    /usr/bin/virsh list | grep running && exit_error "running guest found"
    # See bug 603039, comment 11
    /sbin/modprobe -r kvm_intel
    /sbin/modprobe -r kvm
    /sbin/modprobe kvm allow_unsafe_assigned_interrupts=1
    /sbin/modprobe kvm_intel
}

# Create usb device file
generate_usb_dev_file() {
    /bin/cat > usb_dev.xml << EOX
    <hostdev mode="subsystem" type="usb" managed="yes">
        <source>
            <vendor id='0x$usb_vendor'/>
            <product id='0x$usb_product'/>
        </source>
    </hostdev>
EOX
    append_cleanup "/bin/rm -f usb_dev.xml"
}

# This is harcoded for XML templates for now.
prepare_guest_domains() {
    # Create empty fake disk images
    /bin/dd if=/dev/zero of=$img_path/${dom1}.img bs=1M count=1
    /bin/dd if=/dev/zero of=$img_path/${dom2}.img bs=1M count=1
    /bin/dd if=/dev/zero of=$img_path/${dom3}.img bs=1M count=1
    # Set preconfigured disk image labels for static labeling
    /usr/bin/chcon system_u:object_r:svirt_image_t:s0:c50,c70 $img_path/${dom1}.img
    # Remove disk image at cleanup
    append_cleanup "/bin/rm -f $img_path/${dom1}.img"
    append_cleanup "/bin/rm -f $img_path/${dom2}.img"
    append_cleanup "/bin/rm -f $img_path/${dom3}.img"
}

# Create and start a guest domain. Adds the usb device to the guest domain
# if passed with more then 1 argument
create_guest_domain() {
    if [ ! -z "$2" ] ; then
        /bin/sed '/HOSTDEV_CONFIG/ {r usb_dev.xml
        d}' ${1}-template.xml > ${1}.xml
    else
        /bin/sed 's/HOSTDEV_CONFIG//' ${1}-template.xml > ${1}.xml
    fi

    # remove the xml file and destroy the domain at cleanup
    append_cleanup "/bin/rm -f ${1}.xml"
    prepend_cleanup "/usr/bin/virsh destroy $1"

    /usr/bin/virsh create ${1}.xml
    return $?
}

# Destroy the domain
destroy_guest_domain() {
    local rc=0
    /usr/bin/virsh destroy $1 || ((rc+=1))
    return $rc
}

# Get guest domain pid
get_guest_domain_pid() {
    # Do not continue if there is no such running domain
    /usr/bin/virsh list | grep $1 > /dev/null || \
        exit_error "Domain $1 not found"
    local pid
    for pid in `pgrep qemu-kvm` ; do
        /bin/ps -fp $pid | /bin/grep $1 > /dev/null && echo $pid && break
    done
}

#
# Checks for prerequities
#

check_installed_packages() {
    /bin/rpm -q $1
    return $?
}

check_kvm_modules() {
    # TODO: AMD check
    /sbin/lsmod | /bin/grep kvm_intel
    return $?
}

check_virt_extensions() {
    # For AMD we would checkfor svm
    /bin/grep vmx /proc/cpuinfo
    return $?
}

# Check if USB device has owner and selinux label
# set correctly when static labeling is used
check_usb_device() {
    local owner label
    owner=$(stat -c "%U:%G" /dev/bus/usb/$usb_bus/$usb_device)
    [ $owner != "qemu:qemu" ] && ((rc+=1))

    label=$(stat -c "%C" /dev/bus/usb/$usb_bus/$usb_device)
    echo $label | grep "svirt_image_t:s0:c50,c70" || ((rc+=1))

    return $rc
}

# Check if USB device has owner and selinux label
# set correctly when dynamic labeling is used
# $1 - domain to check
# $2 - yes/no - check for attached(yes)/detached(no) device
check_usb_device_dynamic() {
    local owner label domlabel
    owner=$(stat -c "%U:%G" /dev/bus/usb/$usb_bus/$usb_device)
    perms=$(stat -c "%a" /dev/bus/usb/$usb_bus/$usb_device)
    label=$(stat -c "%C" /dev/bus/usb/$usb_bus/$usb_device)

    # checks for attached device
    if [ "x$2" = "xyes" ]; then
        [ $owner != "qemu:qemu" ] && ((rc+=1))

	    domlabel=$(ps -p $(get_guest_domain_pid $1) -Z | grep -v LABEL \
            | sed 's/svirt_t/svirt_image_t/' | tr ':' ' ' \
            | awk '{printf "%s:%s:%s", $3, $4, $5}')
        echo $label | grep $domlabel || ((rc+=1))

    # check for dettached device
    else
        [ $owner != "root:root" ] && ((rc+=1))
        [ $perms != 664 ] && ((rc+=1))
        echo $label | grep $USB_DEFCON  || ((rc+=1))
    fi

    return $rc
}

# Check if USB device has the owner and SELinux label
# set to the host system and no guest
check_usb_device() {
    local owner label
    owner=$(stat -c "%U:%G" /dev/bus/usb/$usb_bus/$usb_device)
    [ $owner != "qemu:qemu" ] && ((rc+=1))

    label=$(stat -c "%C" /dev/bus/usb/$usb_bus/$usb_device)
    echo $label | grep "svirt_image_t:s0:c50,c70" || ((rc+=1))

    return $rc
}

# Check if USB device cannot be accessed by an rogue VM
# This test expects
rogue_usb_device_access() {
    local runcat roguecat
    roguecat="c1,c2"
    AUDITMARK=$(get_audit_mark)
    chcon -t qemu_exec_t /bin/cat
    # get category of running domain
    runcat=$(ps -p $(get_guest_domain_pid $1) -Z | grep -v LABEL \
        | sed 's/.*\(c[0-9]*,c[0-9]*\).*/\1/')
    # change category if
    [ "x$runcat" = "x$roguecat" ] && roguecat="c1,c5"
    runcon system_u:system_r:svirt_t:s0:$roguecat /bin/cat \
        /dev/bus/usb/$usb_bus/$usb_device
    restorecon -vvF /bin/cat
    augrok --seek=$AUDITMARK type==AVC comm=cat name=$usb_device success=no \
        || exit_fail "Rogue qemu process can read USB device"
}

# Check if the given domains have unique SELinux context
# $1 - first domain
# $2 - second domain
check_guests_labels() {
    local dom1label dom2label
    dom1label=$(ps -p $(get_guest_domain_pid $1) -Z | grep -v LABEL \
            | awk '{printf $1}')
    dom2label=$(ps -p $(get_guest_domain_pid $2) -Z | grep -v LABEL \
            | awk '{printf $1}')

    [ $dom1label = $dom2label ] && return 1

    return 0
}

# USB device handling and individual test sets
#

# Try to attach USB device via attach-device
#
# Params:
#  $1 .. guest domain name where to attach device
attach_usb_device() {
    local rc=0
    /usr/bin/virsh attach-device $1 usb_dev.xml || ((rc+=1))
    sleep 3
    return $rc
}

# Try to detach USB device via detach-device
#
# Params:
#  $1 .. guest domain name from where to detach device
detach_usb_device() {
    local rc=0
    /usr/bin/virsh detach-device $1 usb_dev.xml || ((rc+=1))
    return $rc
}

# Start a guest domain with attached USB device
#
# Params:
#  $1 .. guest domain to start
start_guest_with_usb_device() {
    local rc=0
    create_guest_domain $1 usb_dev.xml || ((rc+=1))
    sleep 3
    return $rc
}

# Start a guest domain without attached USB device
#
# Params:
#  $1 .. guest domain to start
start_guest_without_usb_device() {
    create_guest_domain $1
    return $?
}


#
# Test set functions
#

# Try to attach USB device after boot of VM with static labeling
# Check if attached correctly and no rogue VM can access the device
test_sanity_attach_after_boot() {
    # Positive test - sanity_attach_after_boot
    start_guest_without_usb_device $dom1 || \
        exit_fail "Failed to start guest without assigned USB device"
    relabel_usb_device_files_for_domain $dom1
    attach_usb_device $dom1 || exit_fail "Attach failed"
    check_usb_device || exit_fail "USB permission check failed"
    rogue_usb_device_access $dom1 || \
        exit_fail "Rogue VM can access the attached USB device"
}

# Try to attach USB device after boot of VM with static labeling
# Check if attached correctly and no rogue VM can access the device
test_sanity_attach_on_boot() {
    relabel_usb_device_files_for_domain $dom1
    start_guest_with_usb_device $dom1 || \
        exit_fail "Failed to start guest with assigned USB $usb_device"
    check_usb_device || exit_fail "USB permission check failed"
    rogue_usb_device_access $dom1 || \
        exit_fail "Rogue VM can access the attached USB device"
}

test_sanity_detach_1() {
    relabel_usb_device_files_for_domain $dom1
    start_guest_with_usb_device $dom1 || \
        exit_fail "Failed to start guest with assigned USB $usb_device"
    check_usb_device || exit_fail "USB permission check failed"
    rogue_usb_device_access $dom1 || \
        exit_fail "Rogue VM can access the attached USB device"
    detach_usb_device $dom1 || exit_fail "Detach failed"
}

test_sanity_detach_2() {
    start_guest_without_usb_device $dom1 || \
        exit_fail "Failed to start guest without assigned USB device"
    relabel_usb_device_files_for_domain $dom1
    attach_usb_device $dom1 || exit_fail "Attach failed"
    check_usb_device || exit_fail "USB permission check failed"
    rogue_usb_device_access $dom1 || \
        exit_fail "Rogue VM can access the attached USB device"
    detach_usb_device $dom1 || exit_fail "Detach failed"
}

#
# Test if dynamic labeling works and attached USB device gets the dynamically
# assigned context correctly. Also test if another rogue VM cannot access
# the USB device.
#
test_dynamic_attach_on_boot() {
    start_guest_with_usb_device $dom2 || \
        exit_fail "Failed to start guest with assigned USB $usb_device"
    start_guest_without_usb_device $dom3 || \
        exit_fail "Failed to start guest domain $dom3"
    check_guests_labels $dom2 $dom3 || \
        exit_fail "Domains $dom2 and $dom3 have the same SELinux category"
    check_usb_device_dynamic $dom2 yes || \
        exit_fail "Permission check failed for attached USB device"
    rogue_usb_device_access $dom2 || \
        exit_fail "Rogue VM can access the attached USB device"
    destroy_guest_domain $dom2
    check_usb_device_dynamic $dom2 no || \
        exit_fail "Permission check failed for dettached USB device"
}


#
# Check common prerequisities and prepare test environment
#

if [ -z "$scenario" ] ; then
    echo "Please specify a scenario on the command-line"
    exit 2
fi

reload_kvm_module_for_unsafe_interrupts

check_virt_extensions || exit_error "Missing vmx CPU flag"
check_kvm_modules || exit_error "Missing kvm_intel module"
check_installed_packages $pkg_list || \
    exit_error "Please install all required packages"

prepare_guest_domains
generate_usb_dev_file

# Run specified test

if [ $(type -t test_$scenario) == function ] ; then
    test_$scenario
else
    exit_error "There is no such test scenario ($scenario)"
fi

# Hoooraay ... no failures! We are good to pass ;-)
exit_pass
