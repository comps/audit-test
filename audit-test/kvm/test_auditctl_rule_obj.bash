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

#   test_auditctl_rule_obj.bash
#
#   Assert audit records are generated based on virtual machine resource
#   category label.


source testcase.bash || exit 2

set -x

filename="/var/lib/libvirt/images/testresource.img"

append_cleanup "rm -f $filename"
rm -f $filename
qemu-img create $filename 8G

for context in $contexts; do
	levl=$(echo "$context" | awk 'BEGIN { FS = ":" } ; { print $4 ":" $5 }')

	append_cleanup "auditctl -d exit,always -F arch=b32 -S close -S open -F obj_lev_high=$levl"
	append_cleanup "auditctl -d exit,always -F arch=b64 -S close -S open -F obj_lev_high=$levl"
	auditctl -d exit,always -F arch=b32 -S close -S open -F obj_lev_high=$levl
	auditctl -d exit,always -F arch=b64 -S close -S open -F obj_lev_high=$levl
	auditctl -a exit,always -F arch=b32 -S close -S open -F obj_lev_high=$levl
	auditctl -a exit,always -F arch=b64 -S close -S open -F obj_lev_high=$levl

	chcon $context $filename
	qemu-img info $filename

	expression="obj==$context"

	if [[ $(augrok -c --seek $offset $expression) -eq 0 ]]; then
		exit_fail
	fi
done

exit_pass

#   vim: set noet sw=8 ts=8 tw=0:
