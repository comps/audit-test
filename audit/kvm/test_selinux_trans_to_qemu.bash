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

#   test_selinux_trans_to_qemu.bash
#
#   Assert only processes executing with unconfined_t or virtd_t SELinux types
#   are allowed to transition to qemu_t.


source testcase.bash || exit 2

set -x

allowed=$(sesearch -t qemu_t -c process -p transition --allow)
allowed=$(echo "$allowed" | grep -E "^.*allow")
allowed=$(echo "$allowed" | awk '{ print $2 }')
allowed=$(echo "$allowed" | sed "/lspp_harness_t/d")
allowed_count=$(echo "$allowed" | wc -l)

if [[ $allowed_count -eq 0 ]]; then
	exit_fail
fi

for type in $allowed; do
	if [[ ! "$type" =~ unconfined_t|virtd_t ]]; then
		exit_fail
	fi
done

exit_pass

#   vim: set noet sw=8 ts=8 tw=0:
