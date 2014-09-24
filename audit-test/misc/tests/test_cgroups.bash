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

# sanity-check that a directory contains valid cgroups structure,
# note that this is a best-effort guess
sane_cgroup_mount()
{
    local mnt="$1"
    [ -d "$mnt" \
        -a -f "${mnt}/tasks" -a -w "${mnt}/tasks" \
        -a -f "${mnt}/cgroup.procs" ] || return 1
    return 0
}

# verify that a cgroup is mounted and mount it if not
# please not that we employ various tricks here to make it all work,
# due to how cgroups work as a filesystem - if some other process/user
# combines controllers into a single hierarchy, we're unable to mount
# each controller separately (it can exist only in one instance)
# - ie. RHEL7/systemd combines cpu with cpuacct
mount_cgroup()
{
    local cgroup="$1" mntpoint=

    # first, try to find already mounted cgroup
    # - this works even when the cgroup controller is part of a more
    #   complex hierarchy
    mntpoint=$(cat /proc/mounts | awk -F' ' '{
        if ($3 == "cgroup") {
            split($4, opts, ",");
            for (i in opts) {
                if (opts[i] == wantedcg) {
                    print $2;
                    exit 0;
                }
            }
        }
    }' wantedcg="$cgroup")

    if [ "$mntpoint" ]; then
        sane_cgroup_mount "$mntpoint" || return 1
        echo "$mntpoint"
        return 0
    fi

    # not mounted, try mounting it
    mntpoint=$(mktemp -d)
    prepend_cleanup "umount \"$mntpoint\"; rmdir \"$mntpoint\""
    mount -t cgroup "$cgroup" -o "$cgroup" "$mntpoint" || return 1

    sane_cgroup_mount "$mntpoint" || return 1
    echo "$mntpoint"

    return 0
}


#
# main
#

cgroup="$1"
case "$cgroup" in
    cpu|memory|blkio|devices|freezer) ;;
    *) exit_error "unsupported controller: $1" ;;
esac

# mount the controller and create a custom testing cgroup
base=$(mount_cgroup "$cgroup") || exit_error "failed mounting cgroup fs"
base="${base}/testcg_$RANDOM"
prepend_cleanup "rmdir \"$base\""
mkdir "$base" || exit_error "cannot create cgroup"

# the tests below work by comparing reference runs (outside cgroups) with
# runs under cgroups - the latter ones should be limited by the scope
# of each given cgroup controller

case "$cgroup" in

    cpu)
        # logic: increment counter in an infinite loop, interrupt
        # after a certain time
        quota_us=1000     # 0.001sec CPU time
        period_us=100000  # each 0.1sec real time

        # reference: run at unrestricted speed
        ref=$(timeout --preserve-status -s INT 3 ./cgroup_limits cpu)
        [ $? -eq 0 -a "$ref" ] || exit_fail "reference run failed"

        # limited: run at reduced speed
        echo "$quota_us" > "$base"/cpu.cfs_quota_us || exit_error
        echo "$period_us" > "$base"/cpu.cfs_period_us || exit_error
        limited=$(timeout --preserve-status -s INT 3 ./cgroup_exec "$base"/tasks ./cgroup_limits cpu)
        [ $? -eq 0 -a "$limited" ] || exit_error "limited run failed"

        # result: 'ref' clocked theoretical 0.1sec CPU time each 0.1sec
        # of real time, 'limited' only 1/100th of that, meaning that the
        # resulting counter from 'limit' should be 100 times smaller
        # - accept anything under 1/50th due to scheduler
        ref=$((ref / 50))
        [ "$limited" -lt "$ref" ] || \
            exit_fail "limited run ate more CPU time than expected ($limited/$ref)"
        ;;

    memory)
        # logic: try to allocate twice as much memory as the limit allows
        ceiling=$((2048 * 1024))  # 2 MiB
        limit=$((1024 * 1024))    # 1 MiB

        # reference: allocate all requested memory
        ref=$(./cgroup_limits memory "$ceiling")
        [ $? -eq 0 -a "$ref" = "$ceiling" ] || exit_fail "reference allocation failed"

        # limited: fail allocation or get killed by OOM killer
        echo "$limit" > "$base"/memory.limit_in_bytes || exit_error
        echo "$limit" > "$base"/memory.memsw.limit_in_bytes || exit_error
        limited=$(./cgroup_exec "$base"/tasks ./cgroup_limits memory "$ceiling")

        # result: the proces should be SIGKILLed by an OOM daemon
        # if memory overcommit is enabled, or return 0 as bytes allocated
        # when the allocation failed
        [ $? -eq 137 -o "$limited" = "0" ] || \
            exit_fail "limited run succeeded allocation / was not killed"
        ;;

    blkio)
        # logic: try to read as much blocks as possible from a block device
        # within a given time frame
        bps=40960  # 40k = 10 blocks per second for loop devices
        total=10M

        # NOTE: while we could use files, we would need to ensure O_DIRECT
        # and aligned reads and specific amounts of bytes per read(), along
        # with ensuring that the file is on a block device (not a tmpfs),
        # so just use losetup instead
        backfile=$(mktemp)
        prepend_cleanup "rm -f \"$backfile\""
        dd if=/dev/zero of="$backfile" bs="$total" count=1 || exit_error
        device=$(losetup --show -f "$backfile") || exit_error
        prepend_cleanup "losetup -d \"$device\""
        devnums=$(stat --format='%t:%T' "$device") || exit_error
        # much more accurate measurements: disable readahead
        origra=$(blockdev --getra "$device") || exit_error
        prepend_cleanup "blockdev --setra \"$origra\" \"$device\""
        blockdev --setra 0 "$device" || exit_error

        # reference: read without restrictions
        ref=$(timeout --preserve-status -s INT 1 ./cgroup_limits blkio "$device")
        [ $? -eq 0 -a "$ref" -gt 0 ] || exit_fail "reference read failed"

        # limited: read much less bytes
        echo "$devnums $bps" > "$base"/blkio.throttle.read_bps_device || exit_error
        limited=$(timeout --preserve-status -s INT 1 ./cgroup_exec "$base"/tasks ./cgroup_limits blkio "$device")
        [ $? -eq 0 -a "$limited" -gt 0 ] || exit_fail "limited read failed"

        # result: the limited proces should have read 1sec * 40k worth
        # of data, which is at most 40960/10485760th or ~ 1/256th of the
        # reference process
        # - accept anything under 1/100th
        ref=$((ref / 100))
        [ "$limited" -lt "$ref" ] || \
            exit_fail "limited run ate more blk bandwidth than expected ($limited/$ref)"
        ;;

    devices)
        # logic: try opening a testing device using open(2)
        device="/dev/null"
        devnums=$(stat --format='%t:%T' "$device") || exit_error

        # reference: succeed in opening the device
        ref=$(./cgroup_limits devices "$device")
        [ $? -eq 0 -a "$ref" = "0" ] || exit_fail "reference device open failed"

        # limited: fail at opening the device
        echo "a $devnums rwm" > "$base"/devices.deny || exit_error
        limited=$(./cgroup_exec "$base"/tasks ./cgroup_limits devices "$device")

        # result: the errno returned should be EPERM (1)
        [ $? -eq 0 -a "$limited" = "1" ] || \
            exit_fail "limited run did not fail on device open"
        ;;

    freezer)
        # logic: expect strings within certain time intervals (none will arive
        # if the process is frozen)

        # reference: run without freeze restrictions
        coproc timeout 60 ./cgroup_limits freezer
        freezer1=$(head -n1 <&"${COPROC[0]}")
        kill -INT "$!"  # continue
        freezer2=$(timeout 2 head -n1 <&"${COPROC[0]}") || kill "$!"
        wait "$!"
        [ $? -eq 0 -a "$freezer1" = "freezer1" -a "$freezer2" = "freezer2" ] || \
            exit_fail "reference run failed"

        # limited: freeze before signalling the process to continue
        coproc timeout 60 ./cgroup_exec "$base"/tasks ./cgroup_limits freezer
        freezer1=$(head -n1 <&"${COPROC[0]}")
        echo FROZEN > "$base"/freezer.state
        kill -INT "$!"  # continue
        freezer2=$(timeout 2 head -n1 <&"${COPROC[0]}") || kill "$!"
        echo THAWED > "$base"/freezer.state
        wait "$!"
        [ $? -eq 143 -a "$freezer1" = "freezer1" -a "$freezer2" = "" ] || \
            exit_fail "limited run failed"

        # result: limited run should be killed while still being frozen, never
        # returning 'freezer2' (checked above)
        ;;
esac

exit_pass

# vim: sts=4 sw=4 et :
