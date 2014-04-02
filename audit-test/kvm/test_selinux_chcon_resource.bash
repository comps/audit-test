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

#   test_selinux_chcon_resource.bash
#
#   Assert only superuser is allowed to change virtual machine resource
#   category labels.


source testcase.bash || exit 2

set -x

append_cleanup "killall -9 -u testuser1; userdel -fr testuser1"
append_cleanup "groupdel testuser1"
userdel -fr testuser1
groupdel testuser1
useradd testuser1 -G libvirt

if [[ $? -ne 0 ]]; then
	exit_error
fi

append_cleanup "killall -9 -u testuser2; userdel -fr testuser2"
append_cleanup "groupdel testuser2"
userdel -fr testuser2
groupdel testuser2
useradd testuser2

if [[ $? -ne 0 ]]; then
	exit_error
fi

for i in $(seq $first $last); do
	#   Assert processes executing with qemu_t SELinux are not allowed to
	#   change virtual machine resource category labels.

    offset=$(stat -c '%s' /var/log/audit/audit.log)

    if [[ $PPROFILE == lspp ]]; then
	    eval "runcon -t qemu_t -- chcon -l s0:c1,c3 \$kvm_guest_${i}_resource"
    else
        eval "runcon -u system_u -r system_r -t initrc_t -- runcon -t virtd_t -- \
        runcon -t qemu_t -- chcon -l s0:c1,c3 /var/lib/libvirt/images/KVM-Guest-1.img"
    fi

	if [[ $? -eq 0 ]]; then
		exit_fail
	fi

	# We need to check for svirt_t starting from RHEL7 too because qemu_t became an alias to it
	expression="type==AVC and extra_text=~denied and comm==runcon and scontext=~(qemu_t|svirt_t)"

	if [[ $(augrok -c --seek $offset $expression) -eq 0 ]]; then
		exit_fail
	fi

    if [[ $PPROFILE == lspp ]]; then
	    expression="type==SYSCALL syscall==59 and success==no and comm==runcon and subj=~lspp_harness_t"
    else
	    expression="type==SYSCALL syscall==59 and success==no and comm==runcon and subj=~virtd_t"
    fi

	if [[ $(augrok -c --seek $offset $expression) -eq 0 ]]; then
		exit_fail
	fi

	#   Assert processes executing with svirt_t SELinux are not allowed to
	#   change virtual machine resource category labels.

    offset=$(stat -c '%s' /var/log/audit/audit.log)

    if [[ $PPROFILE == lspp ]]; then
	    eval "runcon -t svirt_t -- chcon -l s0:c1,c3 \$kvm_guest_${i}_resource"
    else
        eval "runcon -u system_u -r system_r -t initrc_t -- runcon -t virtd_t -- \
        runcon -t svirt_t -- chcon -l s0:c1,c3 /var/lib/libvirt/images/KVM-Guest-1.img"
    fi

	if [[ $? -eq 0 ]]; then
		exit_fail
	fi

	expression="type==AVC and extra_text=~denied and comm==runcon and scontext=~svirt_t"

	if [[ $(augrok -c --seek $offset $expression) -eq 0 ]]; then
		exit_fail
	fi

    if [[ $PPROFILE == lspp ]]; then
	    expression="type==SYSCALL syscall==59 and success==no and comm==runcon and subj=~lspp_harness_t"
    else
	    expression="type==SYSCALL syscall==59 and success==no and comm==runcon and subj=~virtd_t"
    fi

	if [[ $(augrok -c --seek $offset $expression) -eq 0 ]]; then
		exit_fail
	fi

	#   Assert non privileged users in libvirt grou are not allowed to
	#   change virtual machine resource category labels.

	eval "/bin/su - testuser1 -c \"chcon -l s0:c1,c3 \$kvm_guest_${i}_resource\""

	if [[ $? -eq 0 ]]; then
		exit_fail
	fi

	#   Assert non privileged users not in libvirt grou are not allowed to
	#   change virtual machine resource category labels.

	eval "/bin/su - testuser2 -c \"chcon -l s0:c1,c3 \$kvm_guest_${i}_resource\""

	if [[ $? -eq 0 ]]; then
		exit_fail
	fi
done

exit_pass

#   vim: set noet sw=8 ts=8 tw=0:
