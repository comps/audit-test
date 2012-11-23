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

#   test_libvirt_label_resource_unique.bash
#
#   Assert resources associated with the processes representing virtual
#   machine environments have different category labels.


source testcase.bash || exit 2

set -x

unique_contexts=$(echo "$contexts" | sort | uniq -u)
unique_contexts_count=$(echo "$contexts" | sort | uniq -u | wc -l)

if [[ $unique_contexts_count -eq 0 ]]; then
	exit_fail
fi

repeated_contexts=$(echo "$contexts" | sort | uniq -d)
repeated_contexts_count=$(echo "$contexts" | sort | uniq -d | wc -l)

if [[ $repeated_contexts_count -ne 0 ]]; then
	exit_fail
fi

exit_pass

#   vim: set noet sw=8 ts=8 tw=0:
