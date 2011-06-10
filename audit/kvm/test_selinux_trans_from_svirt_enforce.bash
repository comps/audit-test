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

#   test_selinux_trans_from_svirt_enforce.bash
#
#   Assert processes executing with svirt_t SELinux type are allowed to
#   transition to ptchown_t and abrt_helper_t only.


source testcase.bash || exit 2

set -x


append_cleanup "rm -f /tmp/true"
rm -f /tmp/true
cp -p /bin/true /tmp/true
chcon -t qemu_exec_t /tmp/true

types=$(seinfo -t)
types=$(echo "$types" | sed "/^Types:/d")
types=$(echo "$types" | sed "/svirt_t/d")
types=$(echo "$types" | sed "/lspp_harness_t/d")
types_count=$(echo "$types" | wc -l)

if [[ $types_count -eq 0 ]]; then
	exit_fail
fi

for type in $types; do
	offset=$(stat -c %s /var/log/audit/audit.log)

	runcon -t svirt_t -- runcon -t $type -- /bin/true

	if [[ $? -eq 0 ]]; then
		if [[ ! "$type" =~ ptchown_t|abrt_helper_t ]]; then
			exit_fail
		fi
	fi

	expression="type==AVC and extra_text=~denied and comm==runcon and scontext=~svirt_t"

	if [[ $(augrok -c --seek $offset $expression) -eq 0 ]]; then
		exit_fail
	fi

	expression="type==SYSCALL syscall==59 and success==no and comm==runcon and subj=~lspp_harness_t"

	if [[ $(augrok -c --seek $offset $expression) -eq 0 ]]; then
		exit_fail
	fi
done

exit_pass

#   vim: set noet sw=8 ts=8 tw=0:
