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

#   test_libvirt_label_match.bash
#
#   Assert processes representing virtual machine environments and its
#   associated resources have the same category label.


source testcase.bash || exit 2

set -x

for i in $(seq $first $last); do
	if [[ $(eval "echo \$kvm_guest_${i}_label_levl") != $(eval "echo \$kvm_guest_${i}_context_levl") ]]; then
		exit_fail
	fi
done

exit_pass

#   vim: set noet sw=8 ts=8 tw=0:
