#!/bin/bash
###############################################################################
#   Copyright (c) 2014 Red Hat, Inc. All rights reserved.
#
#   This program is free software: you can redistribute it and/or modify
#   it under the terms of version 2 the GNU General Public License as
#   published by the Free Software Foundation.
#
#   This program is distributed in the hope that it will be useful,
#   but WITHOUT ANY WARRANTY; without even the implied warranty of
#   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#   GNU General Public License for more details.
#
#   You should have received a copy of the GNU General Public License
#   along with this program.  If not, see <http://www.gnu.org/licenses/>.
###############################################################################

source ../syscalls_ns_functions.bash || exit 2

create_ns mnt
create_env_mnt

# since we're interested only in checking path resolution, use the simplest
# possible case - bindmount of the tested path onto itself

# create swap metadata
dd if=/dev/zero of="${MNT_INFO[0]}" bs=640K count=1
exec_ns dd if=/dev/zero of="${MNT_INFO[1]}" bs=640K count=1
dd if=/dev/zero of="${MNT_INFO[2]}" bs=640K count=1
exec_ns dd if=/dev/zero of="${MNT_INFO[2]}" bs=640K count=1
mkswap "${MNT_INFO[0]}"
exec_ns mkswap "${MNT_INFO[1]}"
mkswap "${MNT_INFO[2]}"
exec_ns mkswap "${MNT_INFO[2]}"

arg="${MNT_INFO[0]}"
# MNT-1A
eval_syscall pass 0 do_swapon "$arg"
eval_syscall pass 0 do_swapoff "$arg"
# MNT-1B, errno 2 = ENOENT
eval_syscall fail 2 exec_ns do_swapon "$arg"
eval_syscall fail 2 exec_ns do_swapoff "$arg"

arg="${MNT_INFO[1]}"
# MNT-2A, errno 2 = ENOENT
eval_syscall fail 2 do_swapon "$arg"
eval_syscall fail 2 do_swapoff "$arg"
# MNT-2B
eval_syscall pass 0 exec_ns do_swapon "$arg"
eval_syscall pass 0 exec_ns do_swapoff "$arg"

arg="${MNT_INFO[2]}"
# MNT-3A
eval_syscall pass 0 dropcap cap_dac_override,cap_dac_read_search \
    do_swapon "$arg"
eval_syscall pass 0 dropcap cap_dac_override,cap_dac_read_search \
    do_swapoff "$arg"
# MNT-3B, errno 1 = EPERM, errno 13 = EACCES
eval_syscall fail 1,13 exec_ns dropcap cap_dac_override,cap_dac_read_search \
    do_swapon "$arg"
eval_syscall fail 1,13 exec_ns dropcap cap_dac_override,cap_dac_read_search \
    do_swapoff "$arg"

exit_pass

# vim: sts=4 sw=4 et :
