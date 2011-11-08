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


#   Skip virtual machine environment installation
#
# skip_installation=0


#   Enable kickstart installation
#
kickstart=1


#   The number of virtual machine environments to install/start prior to
#   execution of test cases. A minimum of two virtual machine environments is
#   required for execution of test cases.
#
process_count=2


#   MLS labels for virtual machine environments
guest_label_1=s0:c50,c70
guest_label_2=s0:c19,c83


#   The amount of memory (total size, in megabytes) to allocate for a virtual
#   machine environment.
#
memory=1024


#   The path to an ISO image to use as the install media for the virtual
#   machine environments.
#
# install_media=


#   The size (total size, in gigabytes) to use if creating a new virtual
#   machine resource.
#
disksize=8


#   The amount of time (total time, in minutes) to wait for a virtual machine
#   environment to complete its install (kickstart installation). Set to zero
#   to wait indefinitely for the virtual machine environment to complete its
#   install.
#
# timeout=


#   Skip test cases that require network functionality.
#
# skip_networking=0


#   Enable SELinux enforcing test cases. Be aware that these tests can take
#   a large amount of time and hang the system.
#
# enable_selinux_enforce=0


#   The amount of time (total time, in minutes) to wait for the virtual
#   machine environment network service to start.
#
# kvm_guest_timeout=


#   The system password of the user that will execute the test cases
#
# password=


#   The system password of the superuser of the virtual machine environments
#
# kvm_guest_password=


#   vim: set noet sw=8 syn=sh ts=8 tw=0:
