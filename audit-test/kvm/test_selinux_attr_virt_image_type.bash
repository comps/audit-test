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

#   test_selinux_attr_virt_image_type.bash
#
#   Assert virt_image_type attribute has virt_content_t, svirt_image_t,
#   virt_image_t and xen_image_t as its associated types only.


source testcase.bash || exit 2

set -x

types=$(seinfo -avirt_image_type -x)
types=$(echo "$types" | sed "/virt_image_type/d")
types_count=$(echo "$types" | wc -l)

if [[ $types_count -eq 0 ]]; then
	exit_fail
fi

for type in $types; do
	if [[ ! "$type" =~ virt_content_t|svirt_image_t|virt_image_t|xen_image_t ]]; then
		exit_fail
	fi
done

exit_pass

#   vim: set noet sw=8 ts=8 tw=0:
