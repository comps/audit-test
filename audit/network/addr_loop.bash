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

####
#
# helper functions
#

function trim_input {
    sed -e 's/[ \t]*#.*//;/^$/d' -e 's/ /-/g'
}

####
#
# main
#

unset addr_tmpl inet_tmpl

# get the parameters
while getopts "A:L:" arg_param; do
    case $arg_param in
	A)
	    addr_tmpl=$OPTARG
	    break
	    ;;
	L)
	    inet_tmpl=$OPTARG
	    ;;
    esac
done

# loop on the addresses

for addr_iter in $(trim_input); do
    export ADDRESS_IPV4=""
    export LBLNET_SVR_IPV4=""
    export ADDRESS_IPV6=""
    export LBLNET_SVR_IPV6=""

    if [[ -n $addr_tmpl ]]; then
	export ADDRESS_IPV4=`echo $addr_iter | cut -d '-' -f 1`
	export ADDRESS_IPV6=`echo $addr_iter | cut -d '-' -f 2`
    elif [[ -n $inet_tmpl ]]; then
	export LBLNET_SVR_IPV4=`echo $addr_iter | cut -d '-' -f 1`
	export LBLNET_SVR_IPV6=`echo $addr_iter | cut -d '-' -f 2`
    fi
    [[ -n $addr_tmpl ]] && cat $addr_tmpl | ../addr_filter.bash
    [[ -n $inet_tmpl ]] && cat $inet_tmpl | ../addr_filter.bash
done
