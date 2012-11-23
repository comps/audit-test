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
# DESCRIPTION: Cgroup device tests for SRF FDP_ACF.1(VIRT)
#

source functions_cgroup_device.bash || exit 2

#
# Global variables
#

# Test scenario to test for by calling corresponding function
scenario=$1

# List of some virt specific packages required for our testing
pkg_list="qemu-kvm libvirt libvirt-client libcgroup"


#
# Test set functions
#

# Basic sanity tests
test_basic_sanity() {
    start_guest $dom1
    destroy_guest_domain $dom1
}

# Test whether it is possible to controll device whitelists
test_devices_controller() {
    start_guest $dom1

    add_to_devices_file $dom1 "$cgroup_block_device_list_entry" deny
    check_cgroup_devices_list $dom1 "$cgroup_block_device_list_entry" && \
        exit_fail "device listed"

    add_to_devices_file $dom1 "$cgroup_block_device_list_entry" allow
    check_cgroup_devices_list $dom1 "$cgroup_block_device_list_entry" || \
        exit_fail "device not listed"

    destroy_guest_domain $dom1
}

# Positive tests on a subgroup for a guest domain with llowed task and device
test_devices_subgroup_devices_allow_task() {
    start_guest $dom1
    create_devices_test_subgroup $dom1/test || exit_error "subgroup"

    # First we need to allow the mknod for parent control group
    add_to_devices_file $dom1 "$cgroup_block_device_list_entry" allow
    check_cgroup_devices_list $dom1 "$cgroup_block_device_list_entry" || \
        exit_fail "block device not listed"

    add_to_devices_file $dom1 "$cgroup_char_device_list_entry" allow
    check_cgroup_devices_list $dom1 "$cgroup_char_device_list_entry" || \
        exit_fail "char device not listed"

    # Now we can allow it for child subgroup for mknod test
    add_to_devices_file $dom1/test "$cgroup_block_device_list_entry" allow
    check_cgroup_devices_list $dom1/test "$cgroup_block_device_list_entry" || \
        exit_fail "block device not listed"

    add_to_devices_file $dom1/test "$cgroup_char_device_list_entry" allow
    check_cgroup_devices_list $dom1/test "$cgroup_char_device_list_entry" || \
        exit_fail "char device not listed"

    export dom1
    bash -c '
    source ./functions_cgroup_device.bash
    rc=0
    # Add current process to the cgroup tasks
    my_pid=`get_my_pid`
    add_proc_to_tasks $my_pid $dom1/test  || let rc+=1
    check_cgroup_tasks $my_pid $dom1/test || let rc+=2
    check_cgroup_procs $my_pid $dom1/test || let rc+=4

    # These should pass
    open_block_device_test || let rc+=8
    rw_block_device_test   || let rc+=16
    char_device_test       || let rc+=32
    mknod_test             || let rc+=64
    echo rc=$rc
    exit $rc
    ' || exit_fail "positive test - block/char device"

    destroy_guest_domain $dom1
}

# Negative tests on a subgroup for a guest domain - allowed task, denied device

test_devices_subgroup_devices_deny_task() {
    start_guest $dom1
    create_devices_test_subgroup $dom1/test || exit_error "subgroup"

    add_to_devices_file $dom1/test "$cgroup_block_device_list_entry" deny
    check_cgroup_devices_list $dom1/test "$cgroup_block_device_list_entry" && \
        exit_fail "block device listed"

    add_to_devices_file $dom1/test "$cgroup_char_device_list_entry" deny
    check_cgroup_devices_list $dom1/test "$cgroup_char_device_list_entry" && \
        exit_fail "char device listed"

    export dom1
    bash -c '
    source ./functions_cgroup_device.bash
    rc=0
    # Add current process to the cgroup tasks
    my_pid=`get_my_pid`
    add_proc_to_tasks $my_pid $dom1/test  || let rc+=1
    check_cgroup_tasks $my_pid $dom1/test || let rc+=2
    check_cgroup_procs $my_pid $dom1/test || let rc+=4

    # Should fail
    open_block_device_test && let rc+=8
    char_device_test       && let rc+=16
    mknod_test             && let rc+=32
    echo rc=$rc
    exit $rc
    ' || exit_fail "negative test - block/char device"

    destroy_guest_domain $dom1
}

test_devices_subgroup_devices_deny_parent() {
    start_guest $dom1

    # A child cgroup can never receive a device access which is denied
    # by its parent.
    add_to_devices_file $dom1 "$cgroup_block_device_list_entry" deny
    check_cgroup_devices_list $dom1 "$cgroup_block_device_list_entry" && \
        exit_fail "block device listed"

    add_to_devices_file $dom1 "$cgroup_char_device_list_entry" deny
    check_cgroup_devices_list $dom1 "$cgroup_char_device_list_entry" && \
        exit_fail "char device listed"

    create_devices_test_subgroup $dom1/test || exit_error "subgroup"

    export dom1
    bash -c '
    source ./functions_cgroup_device.bash
    rc=0
    # Make sure we are not in the tasks/procs
    my_pid=`get_my_pid`
    add_proc_to_tasks $my_pid $dom1/test  || let rc+=1
    check_cgroup_tasks $my_pid $dom1/test || let rc+=2
    check_cgroup_procs $my_pid $dom1/test || let rc+=4

    # Should fail
    open_block_device_test && let rc+=8
    char_device_test       && let rc+=16
    mknod_test             && let rc+=32
    echo rc=$rc
    exit $rc
    ' || exit_fail "negative test - block/char"

    destroy_guest_domain $dom1
}


#
# Check common prerequisities and prepare test environment
#

if [ -z "$scenario" ] ; then
    echo "Please specify a scenario on the command-line"
    exit 2
fi

check_installed_packages $pkg_list || exit_error "Missing required packages"
check_cgroups_availability || exit_error "Congrol groups not ready"

generate_block_dev_file
prepare_guest_domain
prepare_block_device

#
# Run specified test
#

if [ $(type -t test_$scenario) == function ] ; then
    test_$scenario
else
    exit_error "There is no such test scenario ($scenario)"
fi

# Looks like we are good to pass!
exit_pass
