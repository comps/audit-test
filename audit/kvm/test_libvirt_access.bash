#!/usr/bin/env bash
#
#   Copyright 2010, 2011 International Business Machines Corp.
#   Copyright 2010, 2011 Ramon de Carvalho Valle
#
#   This program is free software: you can redistribute it and/or modify
#   it under the terms of the GNU General Public License as published by
#   the Free Software Foundation, either version 2 of the License, or
#   (at your option) any later version.
#
#   This program is distributed in the hope that it will be useful,
#   but WITHOUT ANY WARRANTY; without even the implied warranty of
#   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#   GNU General Public License for more details.
#
#   You should have received a copy of the GNU General Public License
#   along with this program.  If not, see <http://www.gnu.org/licenses/>.
#

#   test_libvirt_access.bash
#
#   Assert only superuser and non privileged users in libvirt group are
#   allowed to access the libvirt daemon and configure virtual machine
#   parameters.


source testcase.bash || exit 2

set -x

append_cleanup "userdel -fr testuser1"
append_cleanup "groupdel testuser1"
userdel -fr testuser1
groupdel testuser1
useradd testuser1 -G libvirt

if [[ $? -ne 0 ]]; then
	exit_error
fi

append_cleanup "userdel -fr testuser2"
append_cleanup "groupdel testuser2"
userdel -fr testuser2
groupdel testuser2
useradd testuser2

if [[ $? -ne 0 ]]; then
	exit_error
fi

#   Assert superuser is allowed to connect to system mode daemon

virsh connect qemu:///system

if [[ $? -ne 0 ]]; then
	exit_fail
fi

#   Assert non privileged users in libvirt group are allowed to connect to
#   system mode daemon.

/bin/su - testuser1 -c "virsh connect qemu:///system"

if [[ $? -ne 0 ]]; then
	exit_fail
fi

#   Assert non privileged users in libvirt group are allowed to connect to
#   system mode daemon in read-only mode.

/bin/su - testuser1 -c "virsh connect qemu:///system --readonly"

if [[ $? -ne 0 ]]; then
	exit_fail
fi

#   Assert non privileged users not in libvirt group are allowed to connect to
#   system mode daemon.

/bin/su - testuser2 -c "virsh connect qemu:///system"

if [[ $? -eq 0 ]]; then
	exit_fail
fi

#   Assert non privileged users not in libvirt group are allowed to connect to
#   system mode daemon in read-only mode.

/bin/su - testuser2 -c "virsh connect qemu:///system --readonly"

if [[ $? -eq 0 ]]; then
	exit_fail
fi

exit_pass

#   vim: set noet sw=8 ts=8 tw=0:
