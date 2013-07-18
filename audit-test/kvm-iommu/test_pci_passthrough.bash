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
# AUTHOR: Eduard Benes <ebenes@redhat.com>
# AUTHOR: Miroslav Vadkerti <mvadkert@redhat.com>
# DESCRIPTION: IOMMU tests for SRF FDP_ACF.1(VIRT)
#
# IMPORTANT NOTES:
# Please note that these tests skip testing of succesful detaching
# of the driver. This is because of bugs, which will not be fixed
# in RHEL6.2:
#
# https://bugzilla.redhat.com/show_bug.cgi?id=736437
# https://bugzilla.redhat.com/show_bug.cgi?id=736423
#

source testcase.bash || exit 2
source pci_device.conf || exit 2
source tp_luks_functions.bash || exit 2

#
# Global variables
#

#
PCI_DEFCON="sysfs_t"

# Test scenario to test for by calling corresponding function
scenario=$1

# List of some virt specific packages required for our testing
pkg_list="qemu-kvm libvirt libvirt-client"

# Memory regions to be looked for in qemu-kvm process maps later
pci_regions=`lspci -vvv -s $pci_device | \
    sed -ne 's/Region.*Memory at \([a-f0-9]\{8\}\) .*/-e \1/p'`

# check if pci_device specified
[[ $pci_device == *X* ]] && \
    exit_error "PCI device in pci_device.conf not specified"

# PCI device variables
pci_device_name="pci_$(echo $pci_device | tr ':.' '_')"
pci_domain="$(echo $pci_device | cut -d: -f1)"
pci_bus="$(echo $pci_device | cut -d: -f2)"
pci_slot="$(echo $pci_device | cut -d: -f3 | sed 's/\..*//')"
pci_function="$(echo $pci_device | cut -d. -f2)"

# Check if the default device driver
# XXX: this test is skipped because of BZ#736437 that is not fixed in RHEL6.2
# pci_driver="$(basename `readlink /sys/bus/pci/devices/${pci_device}/driver`)"
# [ "$pci_driver" == "pci-stub" ] && exit_error \
#   "Something is wrong, pci-stub driver without a device being attached?!"

# Do not change this or make sure to update also domain template XML files.
dom1="guest1"
dom2="guest2"
dom3="guest1-dynamic"
dom4="guest2-dynamic"
img_path="/var/lib/libvirt/images"

# in FIPS mode set gcrypt RNG source to /dev/urandom
FIPS=$(cat /proc/sys/crypto/fips_enabled)
[ "x$FIPS" = "x1" ] && gcrypt_set_rng /dev/urandom

#
# General helper functions
#

reload_kvm_module_for_unsafe_interrupts() {
    # There should be no running guest domain or we will fail
    /usr/bin/virsh list | grep running && exit_error "running guest found"
    # See bug 603039, comment 11
    /sbin/modprobe -r kvm_intel
    /sbin/modprobe -r kvm
    /sbin/modprobe kvm allow_unsafe_assigned_interrupts=1
    /sbin/modprobe kvm_intel
}

# Under MLS we need to call this before passing a device to a domain
relabel_pci_device_files_for_domain() {
    # We don't care about chcons in targeted policy thus will skip the relabel
    sestatus | grep mls || return 0
    # Using context label from guest domain XML template file will allow us
    # to relabel device files also for guests which are not running.
    local category=`grep "<label>" $1-template.xml | \
        sed "s/.*:\(c[0-9]*,c[0-9]*\).*/\1/"`
    /usr/bin/chcon -v system_u:object_r:svirt_image_t:s0:$category \
        /sys/bus/pci/devices/$pci_device/{config,resource*,rom,reset}
    append_cleanup  /sbin/restorecon -RvvvF \
        /sys/bus/pci/devices/$pci_device/{config,resource*,rom,reset}
}

generate_pci_dev_file() {
    /bin/cat > pci_dev.xml << EOX
    <hostdev mode="subsystem" type="pci" managed="yes">
        <source>
            <address domain="0x${pci_domain}" bus="0x${pci_bus}" slot="0x${pci_slot}" function="0x${pci_function}"/>
        </source>
    </hostdev>
EOX
#    append_cleanup "/bin/rm -f pci_dev.xml"
}

# This is harcoded for XML templates for now.
prepare_guest_domains() {
    # Create empty fake disk images
    /bin/dd if=/dev/zero of=$img_path/${dom1}.img bs=1M count=1
    /bin/dd if=/dev/zero of=$img_path/${dom2}.img bs=1M count=1
    /bin/dd if=/dev/zero of=$img_path/${dom3}.img bs=1M count=1
    /bin/dd if=/dev/zero of=$img_path/${dom4}.img bs=1M count=1

    # Set preconfigured disk image labels for static labeling
    /usr/bin/chcon system_u:object_r:svirt_image_t:s0:c50,c70 $img_path/${dom1}.img
    /usr/bin/chcon system_u:object_r:svirt_image_t:s0:c19,c83 $img_path/${dom2}.img

    append_cleanup "/bin/rm -f $img_path/${dom1}.img"
    append_cleanup "/bin/rm -f $img_path/${dom2}.img"
    append_cleanup "/bin/rm -f $img_path/${dom3}.img"
    append_cleanup "/bin/rm -f $img_path/${dom4}.img"
}


# Create and start a guest domain. Adds the pci device to the guest domain
# if passed with more then 1 argument
create_guest_domain() {
    if [ ! -z "$2" ] ; then
        /bin/sed '/HOSTDEV_CONFIG/ {r pci_dev.xml
        d}' ${1}-template.xml > ${1}.xml
    else
        /bin/sed 's/HOSTDEV_CONFIG//' ${1}-template.xml > ${1}.xml
    fi

    append_cleanup "/bin/rm -f ${1}.xml"
    prepend_cleanup "/usr/bin/virsh destroy $1"
    append_cleanup  "/usr/bin/virsh nodedev-reattach $pci_device_name"

    /usr/bin/virsh create ${1}.xml
    return $?
}

destroy_guest_domain() {
    local rc=0
    /usr/bin/virsh destroy $1 || ((rc+=1))
    /usr/bin/virsh nodedev-reattach $pci_device_name || ((rc+=2))
    return $rc
}

get_guest_domain_pid() {
    # Do not continue if there is no such running domain
    /usr/bin/virsh list | grep $1 > /dev/null || return
    local pid
    for pid in `pgrep qemu-kvm` ; do
        /bin/ps -fp $pid | /bin/grep $1 > /dev/null && echo $pid && break
    done
}

#
# Checks for VT-d and IOMMU prerequities. Currently only for Intel.
#

check_installed_packages() {
    /bin/rpm -q $1
    return $?
}

check_kernel_boot_cmdline() {
    /bin/grep intel_iommu=on /proc/cmdline
    return $?
}

check_kvm_modules() {
    # Not sure about AMD here
    /sbin/lsmod | /bin/grep kvm_intel
    return $?
}

check_virt_extensions() {
    # For AMD we would checkfor svm
    /bin/grep vmx /proc/cpuinfo
    return $?
}

check_enabled_iommu() {
    # This might fail if someone cleared the ring buffer content already
    # As a backup we check for similar messge in /var/log/messages
    /bin/dmesg | /bin/grep "IOMMU: enabled" || \
    /bin/grep "IOMMU: enabled" /var/log/messages
    return $?
}


#
# Checks for required object and subject security attributes
#

check_device_driver() {
    driver=$(readlink /sys/bus/pci/devices/${pci_device}/driver)

    # XXX: if the driver is not available we consider this test as PASS
    # this test should be dropped if BZ#736423 is fixed
    [ "x$driver" == "x" ] && return 0

    test "$(basename $driver)" == "$1"
    return $?
}

# Check that a memory region of a PCI device is mapped to qemu-kvm proces
check_proc_pid_maps() {
    # XXX: if the driver is not available we consider this test as PASS
    # this test should be dropped if BZ#736423 is fixed
    driver=$(readlink /sys/bus/pci/devices/${pci_device}/driver)
    [ "x$driver" = "x" ] && return 1

    grep $pci_regions /proc/$1/maps
#   grep pci /proc/$1/maps
    return $?
}

#
# Check if PCI device files are correclty labelled in case when dynamic labeling
# is used. Also check the correct owner of the files.
#
# $1 - domain to check
# $2 - yes/no - check for attached(yes)/detached(no) device
#
check_pci_device_dynamic() {
    local owner label domlabel

    # checks for attached device
    if [ "x$2" = "xyes" ]; then
        domlabel=$(ps -p $(get_guest_domain_pid $1) -Z | grep -v LABEL \
            | sed 's/svirt_t/svirt_image_t/' | tr ':' ' ' \
            | awk '{printf "%s:%s:%s", $3, $4, $5}')

        # go through all required pci device files
        for dfile in $(ls /sys/bus/pci/devices/$pci_device/{config,resource*,rom,reset});
        do
            owner=$(stat -c "%U:%G" $dfile)
            label=$(stat -c "%C" $dfile)

            [ $owner != "qemu:qemu" ] && ((rc+=1))

            echo $label | grep $domlabel || ((rc+=1))
        done
    # check for dettached device
    else
        # go through all required pci device files
        for dfile in $(ls /sys/bus/pci/devices/$pci_device/{config,resource*,rom,reset});
        do
            owner=$(stat -c "%U:%G" $dfile)
            label=$(stat -c "%C" $dfile)

            [ $owner != "root:root" ] && ((rc+=1))

            echo $label | grep $PCI_DEFCON || ((rc+=1))
        done
    fi

    return $rc
}

dump_dmesg_log() {
    /bin/dmesg -c
}


#
# PCI device handling and individual test sets
#

# Params:
#  $1 .. magic scenario number to test for {1,2,3}
#  $2 .. guest domain name where to attach device
#  $3 .. guest domain name to check for process having mem maps
#  $4 .. guest domain name to check for process NOT having mem maps
attach_pci_device() {
    local rc=0
    case $1 in
    1) # Good case - attached correctly
        /usr/bin/virsh attach-device $2 pci_dev.xml || ((rc+=1))
    ;;
    2|3) # Bad case - double attach
        /usr/bin/virsh attach-device $2 pci_dev.xml && ((rc+=1))
    ;;
    *) exit_error "Unknown attach scenario"
    ;;
    esac

    check_device_driver "pci-stub" || ((rc+=2))
    # Look for correctly mapped PCI dev mem regions
    pid_maps="`get_guest_domain_pid $3`"
    pid_nomaps="`get_guest_domain_pid $4`"
    check_proc_pid_maps $pid_maps || ((rc+=4))
    # This only applies to case #3
    [ ! -z $pid_nomaps ] && check_proc_pid_maps $pid_nomaps && ((rc+=8))

    return $rc
}

# Params:
#  $1 .. magic scenario number to test for {1,2,3}
#  $2 .. guest domain name from where to detach device
#  $3 .. guest domain name to check for having process mem maps
detach_pci_device() {
    local rc=0
    case $1 in
    1) # Good case - detached correctly
        /usr/bin/virsh detach-device $2 pci_dev.xml || ((rc+=1))
        check_device_driver $pci_driver || ((rc+=2))
        # Look for NOT mapped PCI dev mem regions
        pid_nomaps="`get_guest_domain_pid $2`"
        check_proc_pid_maps $pid_nomaps && ((rc+=4))
    ;;
    2) # Bad case - double detach
        /usr/bin/virsh detach-device $2 pci_dev.xml && ((rc+=1))
        check_device_driver $pci_driver || ((rc+=2))
        # Look for NOT mapped PCI dev mem regions
        pid_nomaps="`get_guest_domain_pid $2`"
        check_proc_pid_maps $pid_nomaps && ((rc+=4))
    ;;
    3) # Bad case - already in use by other VM
        /usr/bin/virsh detach-device $2 pci_dev.xml && ((rc+=1))
        check_device_driver "pci-stub" || ((rc+=2))
        # Look for mapped PCI dev mem regions
        pid_maps="`get_guest_domain_pid $3`"
        pid_nomaps="`get_guest_domain_pid $2`"
        check_proc_pid_maps $pid_maps || ((rc+=8))
        check_proc_pid_maps $pid_nomaps && ((rc+=4))
    ;;
    *)  exit_error "Unknown detach scenario"
    ;;
    esac

    return $rc
}

start_guest_with_pci_device() {
    local rc=0
    create_guest_domain $1 pci_dev.xml || ((rc+=1))
    check_device_driver "pci-stub" || ((rc+=2))
    pid_maps=`get_guest_domain_pid $1`
    check_proc_pid_maps $pid_maps || ((rc+=4))

    return $rc
#        exit_fail "Failed to start guest with PCI device $pci_device"
}

start_guest_without_pci_device() {
    create_guest_domain $1
    return $?
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

#
# If attached function is on a multifunction device
# this function detaches all other functions from the host
# Please note that this workaround was needed because of this
# BZ#773667 with Broadcom NetExtreme II network card
# This function does not end the testing if the detaching fails
#

detach_multifunction_devices() {
    # check if the the pci function is on a multifunction PCI device
    pcifunction=$(echo pci_${pci_device} | tr ':.' '_')
    pcidevice=$(echo $pcifunction | sed 's/_[0-9]\+$//g')
    if [ $(virsh nodedev-list | grep -c $pcidevice) -gt 1 ]; then
        for dev in $(virsh nodedev-list | grep $pcidevice \
            | grep -v $pcifunction); do
            echo "Detaching PCI device $dev from host"
            virsh nodedev-dettach $dev
            [ $? -eq 0 ] && append_cleanup "virsh nodedev-reattach $dev"
        done
    fi
}


#
# Test set functions
#

test_sanity_attach_after_boot() {
    # Positive test - sanity_attach_after_boot
    start_guest_without_pci_device $dom1 || \
        exit_fail "Failed to start guest without assigned PCI device"
    # Restore /dev/random RNG source if in FIPS mode
    [ "x$FIPS" = "x1" ] && append_cleanup "gcrypt_set_rng /dev/random"
    relabel_pci_device_files_for_domain $dom1
    attach_pci_device 1 $dom1 $dom1 $dom2 || exit_fail "Attach failed"
    destroy_guest_domain $dom1
}

test_sanity_attach_on_boot() {
    # Positive test - sanity_attach_on_boot
    relabel_pci_device_files_for_domain $dom1
    start_guest_with_pci_device $dom1 || \
        exit_fail "Failed to start guest with assigned PCI $pci_device"
    # Restore /dev/random RNG source if in FIPS mode
    [ "x$FIPS" = "x1" ] && append_cleanup "gcrypt_set_rng /dev/random"
    destroy_guest_domain $dom1
}

test_sanity_detach_1() {
    # Positive test - sanity_detach_1
    relabel_pci_device_files_for_domain $dom1
    start_guest_with_pci_device $dom1 || \
        exit_fail "Failed to start guest with assigned PCI $pci_device"
    # Restore /dev/random RNG source if in FIPS mode
    [ "x$FIPS" = "x1" ] && append_cleanup "gcrypt_set_rng /dev/random"
    detach_pci_device 1 $dom1 "" || exit_fail "Detach failed"
    destroy_guest_domain $dom1
}

test_sanity_detach_2() {
    # Positive test - sanity_detach_2
    start_guest_without_pci_device $dom1 || \
        exit_fail "Failed to start guest without assigned PCI device"
    # Restore /dev/random RNG source if in FIPS mode
    [ "x$FIPS" = "x1" ] && append_cleanup "gcrypt_set_rng /dev/random"
    relabel_pci_device_files_for_domain $dom1
    attach_pci_device 1 $dom1 $dom1 $dom2 || exit_fail "Attach failed"
    detach_pci_device 1 $dom1 "" || exit_fail "Detach failed"
    destroy_guest_domain $dom1
}

test_simple_double_attach() {
    # Negative test - simple_double_attach
    start_guest_without_pci_device $dom1 || \
        exit_fail "Failed to start without assigned PCI device"
    # Restore /dev/random RNG source if in FIPS mode
    [ "x$FIPS" = "x1" ] && append_cleanup "gcrypt_set_rng /dev/random"
    relabel_pci_device_files_for_domain $dom1
    attach_pci_device 1 $dom1 $dom1 $dom2 || exit_fail "Attach failed"
    attach_pci_device 2 $dom1 $dom1 $dom2 || exit_fail "We should fail"
    destroy_guest_domain $dom1
}

test_simple_double_detach() {
    # Negative test - simple_double_detach
    relabel_pci_device_files_for_domain $dom1
    start_guest_with_pci_device $dom1 || \
        exit_fail "Failed to start with assigned PCI $pci_device"
    # Restore /dev/random RNG source if in FIPS mode
    [ "x$FIPS" = "x1" ] && append_cleanup "gcrypt_set_rng /dev/random"
    detach_pci_device 1 $dom1 "" || exit_fail "Detach failed"
    detach_pci_device 2 $dom1 "" || exit_fail "We should fail"
    destroy_guest_domain $dom1
}

test_shared_attach_on_boot() {
    # Negative test - shared_attach_on_boot (bz 723535)
    relabel_pci_device_files_for_domain $dom1
    start_guest_with_pci_device $dom1 || \
        exit_fail "Failed to start with assigned PCI"
    relabel_pci_device_files_for_domain $dom2
    start_guest_with_pci_device $dom2 && \
        exit_fail "We should fail"
    # Restore /dev/random RNG source if in FIPS mode
    [ "x$FIPS" = "x1" ] && append_cleanup "gcrypt_set_rng /dev/random"
    destroy_guest_domain $dom1
    destroy_guest_domain $dom2
}

test_shared_attach_used() {
    # Negative test - shared_attach_used
    relabel_pci_device_files_for_domain $dom1
    start_guest_with_pci_device $dom1 || \
        exit_fail "Failed to start with assigned PCI"
    start_guest_without_pci_device $dom2 || \
        exit_fail "Failed to start guest without assigned PCI device"
    # Restore /dev/random RNG source if in FIPS mode
    [ "x$FIPS" = "x1" ] && append_cleanup "gcrypt_set_rng /dev/random"
    relabel_pci_device_files_for_domain $dom2
    attach_pci_device 3 $dom2 $dom1 $dom2 || exit_fail "Should fail to attach"
    destroy_guest_domain $dom1
    destroy_guest_domain $dom2
}

test_shared_detach_used() {
    # Negative test - shared_detach_used
    relabel_pci_device_files_for_domain $dom1
    start_guest_with_pci_device $dom1 || \
        exit_fail "Failed to start with assigned PCI"
    start_guest_without_pci_device $dom2 || \
        exit_fail "Failed to start guest without assigned PCI device"
    # Restore /dev/random RNG source if in FIPS mode
    [ "x$FIPS" = "x1" ] && append_cleanup "gcrypt_set_rng /dev/random"
    relabel_pci_device_files_for_domain $dom2
    detach_pci_device 3 $dom2 $dom1 || exit_fail "We should fail to detach"
    destroy_guest_domain $dom1
    destroy_guest_domain $dom2
}

#
# Test if dynamic labeling works and attached PCI device gets the dynamically
# assigned context correctly.
#
test_dynamic_attach_on_boot() {
    start_guest_with_pci_device $dom3 || \
        exit_fail "Failed to start guest with assigned PCI"
    start_guest_without_pci_device $dom4 || \
        exit_fail "Failed to start guest domain $dom4"
    # Restore /dev/random RNG source if in FIPS mode
    [ "x$FIPS" = "x1" ] && append_cleanup "gcrypt_set_rng /dev/random"
    check_guests_labels $dom3 $dom4 || \
        exit_fail "Domains $dom3 and $dom4 have the same SELinux category"
    check_pci_device_dynamic $dom3 yes || \
        exit_fail "Permission check failed for attached PCI device"
    destroy_guest_domain $dom3
    destroy_guest_domain $dom4
    check_pci_device_dynamic $dom3 no || \
        exit_fail "Permission check failed for dettached PCI device"
}

#
# Check common prerequisities and prepare test environment
#

if [ -z "$scenario" ] ; then
    echo "Please specify a scenario on the command-line"
    exit 2
fi

set_selinux_booleans
reload_kvm_module_for_unsafe_interrupts

check_kernel_boot_cmdline || exit_error "intel_iommu=on is missing"
check_enabled_iommu || \
    exit_error "IOMMU not found in dmesg (please check /var/log/messages)"
check_virt_extensions || exit_error "Missing vmx CPU flag"
check_kvm_modules || exit_error "Missing kvm_intel module"
check_installed_packages $pkg_list || \
    exit_error "Please install all required packages"

prepare_guest_domains
generate_pci_dev_file

detach_multifunction_devices

sestatus | grep mls && \
append_cleanup "restorecon -RvvvF /sys/bus/pci/devices/$pci_device/config"
append_cleanup "restorecon -RvvvF /sys/bus/pci/devices/$pci_device/resource*"
append_cleanup "restorecon -RvvvF /sys/bus/pci/devices/$pci_device/rom"
append_cleanup "restorecon -RvvvF /sys/bus/pci/devices/$pci_device/reset"

# Run specified test

if [ $(type -t test_$scenario) == function ] ; then
    test_$scenario
else
    exit_error "There is no such test scenario ($scenario)"
fi

# Hoooraay ... no failures! We are good to pass ;-)
exit_pass
