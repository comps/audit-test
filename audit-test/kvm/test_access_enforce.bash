#!/usr/bin/env bash
#
#   Copyright 2010, 2011 International Business Machines Corp.
#   Copyright 2010, 2011 Ramon de Carvalho Valle
#   Copyright 2011 Red Hat Corp.
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

#   test_libvirt_label_process.bash
#
#   Assert categories selected by libvirt are associated with the processes
#   representing virtual machine environments.


source testcase.bash || exit 2

set -x

chmod o+x . # Make sure anyone can run "./$script_file"
script_file=$(mktemp test_access_enforce_scriptXXXXXX)
append_cleanup "rm $script_file"

for i in $(seq $first $(($last - 1))); do
	(( j = i + 1 ))
	eval "resource=\$kvm_guest_${j}_resource"
	eval "resource_type=\$kvm_guest_${j}_context_type"
	eval "user=\$kvm_guest_${j}_user"

	append_cleanup "auditctl -d exit,always -F arch=b32 -S close -S open -F obj_type=$resource_type"
	append_cleanup "auditctl -d exit,always -F arch=b64 -S close -S open -F obj_type=$resource_type"
	append_cleanup "auditctl -W /proc/1/mem"
	auditctl -d exit,always -F arch=b32 -S close -S open -F obj_type=$resource_type
	auditctl -d exit,always -F arch=b64 -S close -S open -F obj_type=$resource_type
	auditctl -W /proc/1/mem
	auditctl -a exit,always -F arch=b32 -S close -S open -F obj_type=$resource_type
	auditctl -a exit,always -F arch=b64 -S close -S open -F obj_type=$resource_type
	auditctl -w /proc/1/mem

	cat > $script_file <<EOF
#! /bin/sh
head -c 100 $resource > /dev/null
EOF
	chmod a+rx $script_file
	chcon -t qemu_exec_t $script_file

	# Sanity check: root can read $resource
	offset=$(stat -c %s /var/log/audit/audit.log)
	./$script_file
	if [[ $? -ne 0 ]]; then
		exit_error
	fi
	expression="syscall==2 and success==yes and obj=~$resource_type"
	if [[ $(augrok -c --seek $offset $expression) -eq 0 ]]; then
		exit_error
	fi

	# Guest $i can not read $resource
	eval "context=\$kvm_guest_${i}_label"
	offset=$(stat -c %s /var/log/audit/audit.log)
	/bin/su -s /bin/sh $user -c "runcon $context ./$script_file"
	if [[ $? -eq 0 ]]; then
		exit_fail
	fi
	expression="syscall==2 and success==no and obj=~$resource_type"
	if [[ $(augrok -c --seek $offset $expression) -eq 0 ]]; then
		exit_fail
	fi


	# Guest $i can not write to $resource
	cat > $script_file <<EOF
#! /bin/sh
dd if=/dev/zero bs=1 count=1024 >> $resource
EOF
	chmod a+rx $script_file
	chcon -t qemu_exec_t $script_file

	file_size=$(wc -c < $resource)
	offset=$(stat -c %s /var/log/audit/audit.log)
	/bin/su -s /bin/sh $user -c "runcon $context ./$script_file"
	if [[ $? -eq 0 ]]; then
		exit_fail
	fi
	if [[ $(wc -c < $resource) -ne $file_size ]]; then
		exit_fail
	fi
	expression="syscall==2 and success==no and obj=~$resource_type"
	if [[ $(augrok -c --seek $offset $expression) -eq 0 ]]; then
		exit_fail
	fi


	# Test access to /proc/1/mem; the script only opens the file,
	# reading would read NULL, which fails anyway (we would have to know
	# a valid address).  We are happy enough when open() fails, testing
	# successful read() is not necessary.
	cat > $script_file <<EOF
#! /bin/sh
: < /proc/1/mem
EOF
	chmod a+rx $script_file
	chcon -t qemu_exec_t $script_file
	# Sanity check: root can read /proc/1/mem
	offset=$(stat -c %s /var/log/audit/audit.log)
	./$script_file
	if [[ $? -ne 0 ]]; then
		exit_error
	fi
	expression="syscall==2 and success==yes and name==/proc/1/mem"
	if [[ $(augrok -c --seek $offset $expression) -eq 0 ]]; then
		exit_fail
	fi

	# Guest $i can not read /proc/1/mem
	eval "context=\$kvm_guest_${i}_label"
	offset=$(stat -c %s /var/log/audit/audit.log)
	/bin/su -s /bin/sh $user -c "runcon $context ./$script_file"
	if [[ $? -eq 0 ]]; then
		exit_fail
	fi
	expression="syscall==2 and success==no and name==/proc/1/mem"
	if [[ $(augrok -c --seek $offset $expression) -eq 0 ]]; then
		exit_fail
	fi
done

exit_pass

#   vim: set noet sw=8 ts=8 tw=0:
