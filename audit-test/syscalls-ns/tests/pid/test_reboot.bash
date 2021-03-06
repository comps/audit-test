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

# due to the destructive nature of this syscall, we skip any init ns related
# testing and verify that, when run in a new pid namespace, it doesn't reboot
# the system
# - please note that if this test case reboots the tested system, it should be
#   considered as FAILed

# as the wrapper is immediately killed, no retval/errno checking can be done,
# therefore simply execute the wrapper
exec_ns initwrap do_reboot

exit_pass

# vim: sts=4 sw=4 et :
