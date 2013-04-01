#!/bin/bash
set -x
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
# main
#

unset local_if
unset local_ipv4 remote_ipv4
unset local_ipv6_raw remote_ipv6_raw
unset local_ipv6 remote_ipv6

local_if="$LOCAL_DEV"

local_ipv4="$LOCAL_IPV4"
remote_ipv4="$LBLNET_SVR_IPV4"

local_ipv6_raw="$LOCAL_IPV6"
remote_ipv6_raw="$LBLNET_SVR_IPV6"

local_ipv6="$local_ipv6_raw"
remote_ipv6="$remote_ipv6_raw"

# link-local ipv6 addresses
for i in local_ipv6 remote_ipv6; do
    prefix=$(eval echo \$$i | head -c 4)
    [[ "$prefix" == "fe80" ]] && eval $i=\$$i%$local_if
    unset prefix
done;

#
# do the replacement
#

#
# %LOCAL_*% = always the test machine (TOE)
# %REMOTE_*% = always the network server (NS)
#

sed "s/%LOCAL_IPV4%/$local_ipv4/g; \
    s/%REMOTE_IPV4%/$remote_ipv4/g; \
    s/%LOCAL_IPV6%/$local_ipv6/g; \
    s/%REMOTE_IPV6%/$remote_ipv6/g; \
    s/%LOCAL_IPV6_RAW%/$local_ipv6_raw/g; \
    s/%REMOTE_IPV6_RAW%/$remote_ipv6_raw/g;"
