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

addr_p_tmp=$(mktemp)
addr_c_tmp=$(mktemp)
prepend_cleanup "rm -f \"$addr_p_tmp\" \"$addr_c_tmp\""
PLACEHOLDER_P_BIN="vm_transfer_dummy 1 \"$addr_p_tmp\""
PLACEHOLDER_C_BIN="vm_transfer_dummy 1 \"$addr_c_tmp\""

create_ns pid
create_env_pid  # spawns custom placeholders

# wait for placeholders to write out addresses and read them
sleep 0.1
addr_p=$(<"$addr_p_tmp") || exit_error
addr_c=$(<"$addr_c_tmp") || exit_error
[ "$addr_p" -a "$addr_c" ] || exit_error

arg="${PID_INFO[1]}"
# PID-1A
eval eval_syscall pass 0 do_process_vm_writev "$arg" "$addr_c"
eval eval_syscall pass 0 do_process_vm_readv "$arg" "$addr_c"

arg="${PID_INFO[2]}"
# PID-1B
eval eval_syscall pass 0 exec_ns do_process_vm_writev "$arg" "$addr_c"
eval eval_syscall pass 0 exec_ns do_process_vm_readv "$arg" "$addr_c"

arg="${PID_INFO[0]}"
# PID-2A, errno 3 = ESRCH
eval eval_syscall fail 3 exec_ns do_process_vm_writev "$arg" "$addr_p"
eval eval_syscall fail 3 exec_ns do_process_vm_readv "$arg" "$addr_p"
# PID-2B
eval eval_syscall pass 0 do_process_vm_writev "$arg" "$addr_p"
eval eval_syscall pass 0 do_process_vm_readv "$arg" "$addr_p"

exit_pass

# vim: sts=4 sw=4 et :
