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

#   test_selinux_ipc_signals.bash
#
#   Assert processes representing virtual machine environments do not receive
#   signals from other processes representing virtual machine environments
#   and any other non privileged processes.


source testcase.bash || exit 2

set -x

pids_count=$(echo "$pids" | wc -l)

#   Neither the SIGSTOP signal -which causes a process to halt its execution-
#   nor the SIGKILL signal -which causes a process to exit- can be ignored.

append_cleanup "rm -f /tmp/kill"
rm -f /tmp/kill
cp -p /bin/kill /tmp/kill

chcon -t qemu_exec_t /tmp/kill

for pid in $pids; do
	offset=$(stat -c %s /var/log/audit/audit.log)

	runcon -t svirt_t -- /tmp/kill -9 $pid

	if [[ $? -eq 0 ]]; then
		exit_fail
	fi

	expression="type==AVC and extra_text=~denied and comm==kill and scontext=~svirt_t"

	if [[ $(augrok -c --seek $offset $expression) -eq 0 ]]; then
		exit_fail
	fi

	expression="type==SYSCALL and syscall==62 and success==no and a1==9 and comm==kill and subj=~svirt_t"

	if [[ $(augrok -c --seek $offset $expression) -eq 0 ]]; then
		exit_fail
	fi

	runcon -t svirt_t -- /tmp/kill -19 $pid

	if [[ $? -eq 0 ]]; then
		exit_fail
	fi

	expression="type==AVC and extra_text=~denied and comm==kill and scontext=~svirt_t"

	if [[ $(augrok -c --seek $offset $expression) -lt 2 ]]; then
		exit_fail
	fi

	expression="type==SYSCALL and syscall==62 and success==no and a1==13 and comm==kill and subj=~svirt_t"

	if [[ $(augrok -c --seek $offset $expression) -eq 0 ]]; then
		exit_fail
	fi
done

_pids_count=$(ps -C qemu-kvm -o pid= | wc -l)

if [[ $pids_count -ne $_pids_count ]]; then
	exit_fail
fi

exit_pass

#   vim: set noet sw=8 ts=8 tw=0:
