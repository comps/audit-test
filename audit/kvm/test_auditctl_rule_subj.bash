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

#   test_auditctl_rule_subj.bash
#
#   Assert audit records are generated based on virtual machine category
#   label.


source testcase.bash || exit 2

set -x

true_file=$(mktemp test_auditctl_rule_subj_trueXXXX)
append_cleanup "rm -f $true_file"
rm -f $true_file
cp -p /bin/true $true_file

for label in $labels; do
	levl=$(echo "$label" | awk 'BEGIN { FS = ":" } ; { print $4 ":" $5 }')
	subj_sen=$(echo "$label" | awk 'BEGIN { FS = ":" } ; { print $4 }')
	subj_clr=$(echo "$label" | awk 'BEGIN { FS = ":" } ; { print $5 }')

	append_cleanup "auditctl -d exit,always -F arch=b32 -S close -S open -F subj_clr=$levl"
	append_cleanup "auditctl -d exit,always -F arch=b64 -S close -S open -F subj_clr=$levl"
	append_cleanup "auditctl -d exit,always -F arch=b32 -S close -S open -F subj_sen=$subj_sen,subj_clr=$subj_clr"
	append_cleanup "auditctl -d exit,always -F arch=b64 -S close -S open -F subj_sen=$subj_sen,subj_clr=$subj_clr"
	auditctl -d exit,always -F arch=b32 -S close -S open -F subj_clr=$levl
	auditctl -d exit,always -F arch=b64 -S close -S open -F subj_clr=$levl
	auditctl -d exit,always -F arch=b32 -S close -S open -F subj_sen=$subj_sen,subj_clr=$subj_clr
	auditctl -d exit,always -F arch=b64 -S close -S open -F subj_sen=$subj_sen,subj_clr=$subj_clr
	auditctl -a exit,always -F arch=b32 -S close -S open -F subj_clr=$levl
	auditctl -a exit,always -F arch=b64 -S close -S open -F subj_clr=$levl
	auditctl -a exit,always -F arch=b32 -S close -S open -F subj_sen=$subj_sen,subj_clr=$subj_clr
	auditctl -a exit,always -F arch=b64 -S close -S open -F subj_sen=$subj_sen,subj_clr=$subj_clr

	runcon -l $levl ./$true_file

	subj=$(id -Z | awk 'BEGIN { FS = ":" } ; { print $1 ":" $2 ":" $3 }')
	subj=$subj:$levl

	expression="subj==$subj"

	if [[ $(augrok -c --seek $offset $expression) -eq 0 ]]; then
		exit_fail
	fi
done

exit_pass

#   vim: set noet sw=8 ts=8 tw=0:
