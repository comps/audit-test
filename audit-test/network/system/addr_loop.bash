#!/bin/bash
#
# Multiple address replacement filter for the configuration files
#

###############################################################################
# (c) Copyright Hewlett-Packard Development Company, L.P., 2007
#
#   This program is free software: you can redistribute it and/or modify
#   it under the terms of version 2 the GNU General Public License as
#   published by the Free Software Foundation.
#
#   This program is distributed in the hope that it will be useful,
#   but WITHOUT ANY WARRANTY; without even the implied warranty of
#   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#   GNU General Public License for more details.
#
#   You should have received a copy of the GNU General Public License
#   along with this program.  If not, see <http://www.gnu.org/licenses/>.
###############################################################################

[ $# -lt 1 -o -z "$1" ] && exit 1
template="$1"

for addr_iter in $(sed 's/[ \t]*#.*//;/^$/d;s/ /-/'); do
    export LOCAL_IPV4=`echo $addr_iter | cut -d '-' -f 1`
    export LOCAL_IPV6=`echo $addr_iter | cut -d '-' -f 2`
    cat "$template" | ./addr_filter.bash
done
