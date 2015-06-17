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
# needs to be here, since we can't pass non-trivial variables
# via environment
source ../../cmdline.conf || exit 2

sc="$1"
arg=

[ $# -lt 1 -o -z "$sc" ] && exit_error "$0: syscall not specified"
[ "${cmdline[$sc]}" ] || exit_error "$0: no cmdline for syscall $sc"

create_ns mnt
create_env_mnt

arg="${MNT_INFO[0]}"
# MNT-1A
eval eval_syscall pass 0 "${cmdline[$sc]}"
# MNT-1B, errno 2 = ENOENT
eval eval_syscall fail 2 exec_ns "${cmdline[$sc]}"

arg="${MNT_INFO[1]}"
# MNT-2A, errno 2 = ENOENT
eval eval_syscall fail 2 "${cmdline[$sc]}"
# MNT-2B
eval eval_syscall pass 0 exec_ns "${cmdline[$sc]}"

arg="${MNT_INFO[2]}"
# MNT-3A
eval eval_syscall pass 0 \
    dropcap cap_dac_override,cap_dac_read_search "${cmdline[$sc]}"
# MNT-3B, errno 1 = EPERM, errno 13 = EACCES
eval eval_syscall fail 1,13 \
    exec_ns dropcap cap_dac_override,cap_dac_read_search "${cmdline[$sc]}"

exit_pass

# vim: sts=4 sw=4 et :
