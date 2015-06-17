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

create_ns uts
create_env_uts

test_name="testname${RANDOM}"

# modify new namespace
exec_ns hostname "$test_name"
exec_ns domainname "$test_name"

tmp=$(mktemp)
prepend_cleanup "rm -f \"$tmp\""

# UTS-1A
eval_syscall pass 0 do_uname > "$tmp"
hostname=$(grep 'nodename' < "$tmp" | sed 's/^nodename://')
domainname=$(grep 'domainname' < "$tmp" | sed 's/^domainname://')
[ "$hostname" -a "$domainname" ] || exit_error
[ "$hostname" = "${UTS_INFO[0]}" ] || exit_fail
[ "$domainname" = "${UTS_INFO[1]}" ] || exit_fail

# UTS-1B
eval_syscall pass 0 exec_ns do_uname > "$tmp"
hostname=$(grep 'nodename' < "$tmp" | sed 's/^nodename://')
domainname=$(grep 'domainname' < "$tmp" | sed 's/^domainname://')
[ "$hostname" -a "$domainname" ] || exit_error
[ "$hostname" = "$test_name" ] || exit_fail
[ "$domainname" = "$test_name" ] || exit_fail

exit_pass

# vim: sts=4 sw=4 et :
