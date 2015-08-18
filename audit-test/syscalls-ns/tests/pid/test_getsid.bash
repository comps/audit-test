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

create_ns pid
create_env_pid

# since the namespace already contains an init (pid 1), the process space
# will look like this upon the do_getsid command:
#
# +----------+        +----------+        +----------+
# | pid 1234 |        | pid 1235 |        | pid 1236 |
#========================================================================
# | pid 0    |        | pid 0    |        | pid 0    |
# +----------+        +----------+        +----------+
#        |                   |                   |
#        v                   v                   v
#    +-------+           +-------+           +-------+
#    | pid 1 |           | pid 2 |           | pid 3 |
#    +-------+           +-------+           +-------+
#     (init)          (placeholder C)       (do_getsid)
#
# since the do_getsid process is spawned directly from pid 0, it should
# have sid == 0 in the child as well

# PID-3A
eval_syscall pass 0 do_getsid
sid="${EVAL_SYSCALL_RESULT[1]}"
[ "$sid" -ge "100" ] || exit_fail

# PID-3B
eval_syscall pass 0 exec_ns do_getsid
sid="${EVAL_SYSCALL_RESULT[1]}"
[ "$sid" -eq "0" ] || exit_fail  # 0 < 100

exit_pass

# vim: sts=4 sw=4 et :
