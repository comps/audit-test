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

# since pivot_root is not a safe thing to do in a test, we're passing a regular
# file both as the source *and* the destination, expecting ENOTDIR as our "pass"
# case, meaning no EPERM or ENOENT occurred

arg="${MNT_INFO[0]}"
# MNT-1A, errno 20 = ENOTDIR
eval_syscall fail 20 do_pivot_root "$arg" "$arg"
# MNT-1B, errno 2 = ENOENT
eval_syscall fail 2 exec_ns do_pivot_root "$arg" "$arg"

arg="${MNT_INFO[1]}"
# MNT-2A, errno 2 = ENOENT
eval_syscall fail 2 do_pivot_root "$arg" "$arg"
# MNT-2B, errno 20 = ENOTDIR
eval_syscall fail 20 exec_ns do_pivot_root "$arg" "$arg"

arg="${MNT_INFO[2]}"
# MNT-3A, errno 20 = ENOTDIR
eval_syscall fail 20 dropcap cap_dac_override,cap_dac_read_search \
    do_pivot_root "$arg" "$arg"
# MNT-3B, errno 1 = EPERM, errno 13 = EACCES
eval_syscall fail 1,13 exec_ns dropcap cap_dac_override,cap_dac_read_search \
    do_pivot_root "$arg" "$arg"

exit_pass

# vim: sts=4 sw=4 et :
