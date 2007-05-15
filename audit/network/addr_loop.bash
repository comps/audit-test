#!/bin/bash
#
# Multiple address replacement filter for the configuration files
#

###############################################################################
# (c) Copyright Hewlett-Packard Development Company, L.P., 2007
#
#   This program is free software;  you can redistribute it and/or modify
#   it under the terms of the GNU General Public License as published by
#   the Free Software Foundation; either version 2 of the License, or
#   (at your option) any later version.
#
#   This program is distributed in the hope that it will be useful,
#   but WITHOUT ANY WARRANTY;  without even the implied warranty of
#   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See
#   the GNU General Public License for more details.
#
#   You should have received a copy of the GNU General Public License
#   along with this program;  if not, write to the Free Software
#   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
###############################################################################

####
# 
# helper functions
#

function trim_input {
    sed -e 's/[ \t]*#.*//;/^$/d'
}

function detect_addr_family {
    ([[ $1 == *:* ]] && echo "ipv6") || ([[ $1 == *.* ]] && echo "ipv4")
}

####
# 
# main
#

unset addr_tmpl inet_tmpl inet6_tmpl

# get the parameters
while getopts "A:4:6:" arg_param; do
    case $arg_param in
	A)
	    addr_tmpl=$OPTARG
	    ;;
	4)
	    inet_tmpl=$OPTARG
	    ;;
	6)
	    inet6_tmpl=$OPTARG
	    ;;
    esac
done

# loop on the addresses
for addr_iter in $(trim_input); do
    export ADDRESS_IPV4=""
    export LBLNET_SVR_IPV4=""
    export ADDRESS_IPV6=""
    export LBLNET_SVR_IPV6=""
    case $(detect_addr_family $addr_iter) in
	ipv4)
	    if [[ -n $addr_tmpl ]]; then
		export ADDRESS_IPV4=$addr_iter
		cat $addr_tmpl | ../addr_filter.bash
	    elif [[ -n $inet_tmpl ]]; then
		export LBLNET_SVR_IPV4=$addr_iter
		cat $inet_tmpl | ../addr_filter.bash
	    fi
	    ;;
	ipv6)
	    if [[ -n $addr_tmpl ]]; then
		export ADDRESS_IPV6=$addr_iter
		cat $addr_tmpl | ../addr_filter.bash
	    elif [[ -n $inet6_tmpl ]]; then
		export LBLNET_SVR_IPV6=$addr_iter
		cat $inet6_tmpl | ../addr_filter.bash
	    fi
	    ;;
    esac
done
