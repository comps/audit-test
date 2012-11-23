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

#   test_libvirt_label_process.bash
#
#   Assert categories selected by libvirt are associated with the processes
#   representing virtual machine environments.


source testcase.bash || exit 2

set -x

for i in $(seq $first $last); do
	config_label=$(eval "echo \$(sed -n '/<label>/p' \$kvm_guest_${i}_live_config)")
	config_label=$(echo "$config_label" | sed 's/<label>//')
	config_label=$(echo "$config_label" | sed 's/<\/label>//')
	config_label=$(echo "$config_label" | sed 's/^[\t ]*//')
	config_label=$(echo "$config_label" | sed 's/[\t ]*$//')

	if [[ "$config_label" != $(eval "echo \$kvm_guest_${i}_label") ]]; then
		exit_fail
	fi
done

exit_pass

#   vim: set noet sw=8 ts=8 tw=0:
