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

create_ns ipc
create_env_mq

# since mq_unlink removes mq objects on success, test failures of each object
# first, before trying the success cases

# IPC-1B, errno 2 = ENOENT
eval_syscall fail 2 exec_ns do_mq_unlink "${MQ_INFO[0]}"
# IPC-1A
eval_syscall pass 0 do_mq_unlink "${MQ_INFO[0]}"

# IPC-2A, errno 2 = ENOENT
eval_syscall fail 2 do_mq_unlink "${MQ_INFO[1]}"
# IPC-2B
eval_syscall pass 0 exec_ns do_mq_unlink "${MQ_INFO[1]}"

# IPC-3B is not tested for this syscall due to the root directory being owned
# by the same user that created the shared object (root, uid 0), meaning that
# the unlink operation will always succeed no matter the owner or access perms
# on the shared object itself, dropcap doesn't help either
# - the correct behavior is tested elsewhere (DAC)

# IPC-3B, errno 1 = EPERM, errno 13 = EACCES
#eval_syscall fail 1,13 exec_ns dropcap cap_dac_override,cap_dac_read_search \
#    do_mq_unlink "${MQ_INFO[2]}"
# IPC-3A
eval_syscall pass 0 dropcap cap_dac_override,cap_dac_read_search \
    do_mq_unlink "${MQ_INFO[2]}"

exit_pass

# vim: sts=4 sw=4 et :
