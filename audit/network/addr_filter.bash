#!/bin/bash
#
# Address replacement filter for the configuration files
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

function get_ipv6_prefix {
    if [[ -n $LBLNET_SVR_IPV6 ]]; then
	echo $LBLNET_SVR_IPV6 | \
	    awk 'BEGIN { FS = ":" } { print $1":"$2":"$3":"$4":" }'
    elif [[ -n $LBLNET_PREFIX_IPV6 ]]; then
	echo $LBLNET_PREFIX_IPV6 | sed 's/:\/[0-9]*//;s/:0*/:/g;'
    else
	ip -o -f inet6 addr show scope global | \
	    awk 'BEGIN { FS = "[ \t]*|[ \t\\/]+" } { print $4 }' | \
	    awk 'BEGIN { FS = ":" } { print $1":"$2":"$3":"$4":" }' | \
	    head -n 1
    fi
}

function get_ipv6_iface {
    declare prefix=$(get_ipv6_prefix)
    ip -o -f inet6 addr show scope global | \
	awk 'BEGIN { FS = "[ \t]*|[ \t\\/]+" } { print $2 }' | \
	grep $prefix | head -n 1
}

function get_ipv4_addr {
    ip -o -f inet addr show scope global | head -n 1 | \
    awk 'BEGIN { FS = "[ \t]*|[ \t\\/]+" } { print $4 }'
}

function get_ipv6_addr {
    declare prefix=$(get_ipv6_prefix)
    ip -o -f inet6 addr show scope global | \
	awk 'BEGIN { FS = "[ \t]*|[ \t\\/]+" } { print $4 }' | \
	grep $prefix | head -n 1
}

####
# 
# main
#

unset local_ipv4 remote_ipv4 address_ipv4

unset local_ipv6_if
unset local_ipv6 remote_ipv6 address_ipv6
unset local_ipv6_raw remote_ipv6_raw address_ipv6_raw

unset address address_raw

#
# get ipv4 addresses
#

local_ipv4="$(get_ipv4_addr)"
remote_ipv4="$LBLNET_SVR_IPV4"
address_ipv4="$ADDRESS_IPV4"

#
# get ipv6 addresses
#

# interface/scope
local_ipv6_if="$(get_ipv6_iface)"

# raw addresses
local_ipv6_raw="$(get_ipv6_addr)"
remote_ipv6_raw="$LBLNET_SVR_IPV6"
address_ipv6_raw="$ADDRESS_IPV6"

# adjust link-local addresses
if [[ ${local_ipv6_raw/:*/} == "fe80" ]]; then
    # link-local address, add a scope
    local_ipv6="$local_ipv6_raw%$local_ipv6_if"
else
    # non link-local, assume global address and just use it
    local_ipv6="$local_ipv6_raw"
fi
if [[ ${remote_ipv6_raw/:*/} == "fe80" ]]; then
    # link-local address, add a scope
    remote_ipv6="$remote_ipv6_raw%$local_ipv6_if"
else
    # non link-local, assume global address and just use it
    remote_ipv6="$remote_ipv6_raw"
fi
if [[ ${address_ipv6_raw/:*/} == "fe80" ]]; then
    # link-local address, add a scope
    address_ipv6="$address_ipv6_raw%$local_ipv6_if"
else
    # non link-local, assume global address and just use it
    address_ipv6="$address_ipv6_raw"
fi

#
# generate the generic %ADDRESS[_RAW]% if possible
#

if [[ -n $address_ipv6 && -z $address_ipv4 ]]; then
    address="$address_ipv6"
    address_raw="$address_ipv6_raw"
elif [[ -z $address_ipv6 && -n $address_ipv4 ]]; then
    address="$address_ipv4"
fi

#
# do the replacement
#

sed "s/%LOCAL_IPV4%/$local_ipv4/g; \
    s/%REMOTE_IPV4%/$remote_ipv4/g; \
    s/%ADDRESS_IPV4%/$address_ipv4/g; \
    s/%LOCAL_IPV6%/$local_ipv6/g; \
    s/%REMOTE_IPV6%/$remote_ipv6/g; \
    s/%ADDRESS_IPV6%/$address_ipv6/g; \
    s/%LOCAL_IPV6_RAW%/$local_ipv6_raw/g; \
    s/%REMOTE_IPV6_RAW%/$remote_ipv6_raw/g; \
    s/%ADDRESS_IPV6_RAW%/$address_ipv6_raw/g; \
    s/%ADDRESS%/$address/g; \
    s/%ADDRESS_RAW%/$address_raw/g;"
