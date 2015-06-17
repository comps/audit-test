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

msqid_initns=$(do_msgget "${IPC_INFO[0]}" 0) || exit_error
msqid_newns=$(exec_ns do_msgget "${IPC_INFO[1]}" 0) || exit_error

# IPC-1A
eval_syscall pass 0 do_msgsnd "$msqid_initns" 1 "testmsg"
eval_syscall pass 0 do_msgrcv "$msqid_initns" 1
# IPC-1B, errno 22 = EINVAL, errno 43 = EIDRM
eval_syscall fail 22,43 exec_ns do_msgsnd "$msqid_initns" 1 "testmsg"
eval_syscall fail 22,43 exec_ns do_msgrcv "$msqid_initns" 1

# IPC-2A, errno 22 = EINVAL, errno 43 = EIDRM
eval_syscall fail 22,43 do_msgsnd "$msqid_newns" 1 "testmsg"
eval_syscall fail 22,43 do_msgrcv "$msqid_newns" 1
# IPC-2B
eval_syscall pass 0 exec_ns do_msgsnd "$msqid_newns" 1 "testmsg"
eval_syscall pass 0 exec_ns do_msgrcv "$msqid_newns" 1

# due to the nature of sysv ipc ids (msqid,semid,shmid), IPC-3 is not tested
# here - see README of this bucket for more info

exit_pass

# vim: sts=4 sw=4 et :
