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

#   test_selinux_trans_to_svirt_enforce.bash
#
#   Assert only processes executing with unconfined_t or virtd_t SELinux types
#   are allowed to transition to svirt_t.


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

permissive_types=$(seinfo --permissive)
permissive_types=$(echo "$permissive_types" | sed "/^Permissive Types:/d")
permissive_types_count=$(echo "$permissive_types" | wc -l)

for permissive_type in $permissive_types; do
	types=$(echo "$types" | sed "/$permissive_type/d")
done

if [[ $types_count -eq 0 ]]; then
	exit_fail
fi

for type in $types; do
	offset=$(stat -c %s /var/log/audit/audit.log)

	runcon -t $type -- runcon -t svirt_t -- /tmp/true

	if [[ $? -eq 0 ]]; then
		if [[ ! "$type" =~ unconfined_t|virtd_t ]]; then
			exit_fail
		fi
	fi

	expression="type==AVC and extra_text=~denied and comm==runcon and scontext=~$type"

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
