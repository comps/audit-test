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
create_env_ipc

shmid_initns=$(do_shmget "${IPC_INFO[0]}" 0) || exit_error
shmid_newns=$(exec_ns do_shmget "${IPC_INFO[1]}" 0) || exit_error

# IPC-1A
eval_syscall pass 0 do_shmat "$shmid_initns" read
# IPC-1B, errno 22 = EINVAL, errno 43 = EIDRM
eval_syscall fail 22,43 exec_ns do_shmat "$shmid_initns" read

# IPC-2A, errno 22 = EINVAL, errno 43 = EIDRM
eval_syscall fail 22,43 do_shmat "$shmid_newns" read
# IPC-2B
eval_syscall pass 0 exec_ns do_shmat "$shmid_newns" read

# due to the nature of sysv ipc ids (msqid,semid,shmid), IPC-3 is not tested
# here - see README of this bucket for more info

# IPC-3A
#eval_syscall pass 0 dropcap cap_ipc_owner do_shmat "" read
# IPC-3B, errno 1 = EPERM, errno 13 = EACCES
#eval_syscall fail 1,13 exec_ns dropcap cap_ipc_owner do_shmat "" read

exit_pass

# vim: sts=4 sw=4 et :
