#!/bin/bash
# =============================================================================
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
# =============================================================================
#
# It is important to note that prior to running the tests below the
# system must be configured using the configuration templates in the
# "network/system" directory as directed by the test plan.  Failure to
# configure the system correctly will result in test failures.
#
## PROGRAM:     ipsec
## PURPOSE:
## Verify that the ipsec daemon correctly negotiates IPsec SAs with remote
## hosts and that when these SAs are added to the kernel's SAD the correct
## audit records are generated.  There is also a test case to verify that the
## SAs are removed correctly but this is a duplicate of the ip xfrm trusted
## program test.  If either the SA is not created or the audit record is
## missing when ipsec negotiates a new SA the test fails.  The test procedure
## is as follows:
##  1. Flush any existing SAs from the kernel's SAD
##  2. Attempt to establish a new SA using ipsec by talking to a remote
##     test driver over a connection which is configured to require IPsec
##     protection
##  3. Verify the SA was created and the audit trail is correct
##  4. Remove the SA from the kernel's SAD
##  5. Verify the SA was removed and an audit record was generated
## TESTCASE:    negotiate a SA with ipsec
## TESTCASE:    remove the SAs

source testcase.bash || exit 2

######################################################################
# global variables
######################################################################

unset log_mark
unset ip_src ip_dst

######################################################################
# helper functions
######################################################################

#
# get_ip_addr - Get the local system's glboal IPv4 or IPv6 address
#
# INPUT
# X - 4 for IPv4 or 6 for IPv6
#
# OUTPUT
# Writes the first global IPvX address on the local system to stdout
#
# DESCRIPTION
# This function queries the local system, through the "ip" command, for a list
# of global IPvX addresses, it then selects the first address in the list and
# writes it to stdout.
#
function get_ip_addr {
    if [ $1 == "4" ]; then
	ip -o -f inet addr show scope global | head -n 1 | \
	    awk 'BEGIN { FS = "[ \t]*|[ \t\\/]+" } { print $4 }'
    elif [ $1 == "6" ]; then
	ip -o -f inet6 addr show scope global to $LBLNET_PREFIX_IPV6 | head -n 1 | \
	    awk 'BEGIN { FS = "[ \t]*|[ \t\\/]+" } { print $4 }'
    else
        die "error: expected parameter 4 | 6 not given"
    fi
}

#
# normalize_addr - Add leading zeros to a comparessed IPv6 address
#
# INPUT
# IPv6 address
#
# OUTPUT
# Writes the normalized IPv6 address to stdout
#
# DESCRIPTION
# This function add leading zeros to a compressed IPv6 address,
# e.g. 2620:52:0:2223:216:3eff:fe00:28 will be updated to
# 2620:52:0000:2223:0216:3eff:fe00:0028
#
function normalize_addr {
    addr=$1
    while [ $[`echo $addr | sed 's/[^:]//g' | wc -m`-1] -lt 7 ]; do
    	addr=`echo $addr | sed 's/::/:0000::/g'`
    done

    echo $addr | sed -e 's/:\([0-9a-f]\):/:000\1:/g' \
	-e 's/:\([0-9a-f][0-9a-f]\):/:00\1:/g' \
	-e 's/:\([0-9a-f][0-9a-f][0-9a-f]\):/:0\1:/g' \
	-e 's/:\([0-9a-f]\)$/:000\1/g' \
	-e 's/:\([0-9a-f][0-9a-f]\)$/:00\1/g' \
	-e 's/:\([0-9a-f][0-9a-f][0-9a-f]\)$/:0\1/g' \
	-e 's/::/:0000:/g' \
	-e 's/^:/0000:/g' \
	-e 's/:$/:0000/g'
}

######################################################################
# functions
######################################################################

#
# ipsec_add - Attempt to negotiate a new IPsec SA using ipsec
#
# INPUT
# 4 for IPv4 or 6 for IPv6 test
# none
#
# OUTPUT
# none
#
# DESCRIPTION
# This function attempts to negotiate a IPsec SA with a remote node using the
# "ipsec" daemon.  The function does this by using a test driver on the remote
# node which the function configures to listen on IPv4 or IPv6 TCP port 4300
# (given by parameter X).  Once the remote test driver is waiting for new
# connections the function tries to connect to the remote test driver which
# triggers a SPD rule in the IPsec subsystem which sends a SA "acquire" message
# to the "ipsec" daemon which then attempts to negotiate an IPsec SA with the
# remote host.  If the "ipsec" deamon is unable to negotiate a SA with the
# remote host the connection will fail.  If this function can not setup the
# remote test driver or initiate a connection to the remote test driver it
# will fail, calling exit_error() in the process.
#
function ipsec_add {

    if which nc >& /dev/null; then
        cmd_nc="nc -$1 -w 30 -v "
    else
        die "error: nc not installed"
    fi

    # do the setup
    if [ $1 == "6" ]; then
	runcon -t lspp_test_netlabel_t -l SystemLow -- \
	    $cmd_nc $ip_dst 4000 <<< "recv:ipv6,tcp,4300,0;" &
    elif [ $1 == "4" ]; then
	runcon -t lspp_test_netlabel_t -l SystemLow -- \
	    $cmd_nc $ip_dst 4001 <<< "recv:ipv4,tcp,4300,0;" &
    else
        die "error: expected parameter 4 | 6 not given"
    fi
    # configure the remote system (try twice to allow for IKE negotiation)
    runcon -t lspp_harness_t -l SystemLow -- \
	$cmd_nc $ip_dst 4300 <<< "Hello"
    [[ $? == 0 ]] && return
    sleep 2
    runcon -t lspp_harness_t -l SystemLow -- \
	$cmd_nc $ip_dst 4300 <<< "Hello"
    [[ $? != 0 ]] && exit_error "unable to establish a SA"
}

#
# ipsec_remove - Remove all IPsec SAs on the system
#
# INPUT
# none
#
# OUTPUT
# none
#
# DESCRIPTION
# This function attempts to flush/remove all SAs from the kernel IPsec
# subsystem, including the SAs negotiated in the ipsec_add() function.  If this
# function can not flush/remove all the SAs from the kernel it will call the
# exit_error() function to signify failure.
#
function ipsec_remove {
    # remove the SA
    ip xfrm state flush
    [[ $? != 0 ]] && exit_error "unable to remove the SA"
}

#
# ipsec_add_verify - Verify that ipsec did establish a new SA
#
# INPUT
# none
#
# OUTPUT
# none
#
# DESCRIPTION
# This function queries the kernel's SAD to see if the ipsec_add() function
# was successful in establishing a SA.  In addition this function checks to see
# if an audit record was generated when the SA was established.  If either the
# SA or the audit record is missing this function fails and calls the
# exit_fail() function.
#
function ipsec_add_verify {
    # check the SA
    ip xfrm state | grep -q "proto ah .* mode transport" || \
	exit_fail "failed to add the SA"
    augrok --seek=$log_mark type==MAC_IPSEC_EVENT op=SAD-add \
	sec_alg=1 sec_doi=1 sec_obj=staff_u:lspp_test_r:lspp_harness_t:s0 \
	src=$ip_src dst=$ip_dst res=1 || \
	exit_fail "missing audit record"
}

#
# ipsec_remove_verify - Verify that the SAs have been removed
#
# INPUT
# none
#
# OUTPUT
# none
#
# DESCRIPTION
# This function queries the kernel's SAD to make sure the SAs have been removed
# and that an audit record was generated for the SAs established by the
# ipsec_add() function.  If either any SAs are found or the SA removal audit
# records are missing the function fails and the exit_fail() function is
# called.
#
function ipsec_remove_verify {
    # check the SA
    ip xfrm state | grep -q "proto esp .* mode transport" && \
	exit_fail "failed to remove the SA"
    augrok --seek=$log_mark type==MAC_IPSEC_EVENT op=SAD-delete \
	sec_alg=1 sec_doi=1 sec_obj=staff_u:lspp_test_r:lspp_harness_t:s0 \
	src=$ip_src dst=$ip_dst res=1 || \
	exit_fail "missing audit record"
}

######################################################################
# main
######################################################################

set -x

if [ $1 == "6" ]; then
    [[ -n $(eval echo \$LBLNET_SVR_IPV6) ]] || exit_error

    # setup the global variables
    ip_src=$(normalize_addr $(get_ip_addr $1))
    ip_dst=$(normalize_addr $(eval echo \$LBLNET_SVR_IPV6))
elif [ $1 == "4" ]; then
    [[ -n $(eval echo \$LBLNET_SVR_IPV4) ]] || exit_error

    # setup the global variables
    ip_src=$(get_ip_addr $1)
    ip_dst=$LBLNET_SVR_IPV4
else
        die "error: expected parameter 4 | 6 not given"
fi

ip xfrm state flush || exit_error

# mark the log for augrok later
log_mark=$(stat -c %s $audit_log)

# attempt to negotiate a SA using ipsec and verify the results
ipsec_add $1
ipsec_add_verify

# attempt to remove the SA and verify the results
ipsec_remove
ipsec_remove_verify

# if we made it this far everything is okay
exit_pass
