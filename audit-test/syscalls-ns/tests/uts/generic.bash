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

create_ns uts
create_env_uts

test_name="testname${RANDOM}"

# modify new namespace
arg="$test_name"
eval eval_syscall pass 0 exec_ns "${cmdline[$sc]}"

# UTS-1A
[ "$(hostname)" = "${UTS_INFO[0]}" ] || exit_fail
[ "$(domainname)" = "${UTS_INFO[1]}" ] || exit_fail

# UTS-1B
# (at least one of host/domain name has to be changed)
[ "$(exec_ns hostname)" = "$test_name" -o \
  "$(exec_ns domainname)" = "$test_name" ] || exit_fail

exit_pass

# vim: sts=4 sw=4 et :
