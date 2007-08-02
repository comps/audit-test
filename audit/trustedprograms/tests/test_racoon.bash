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
## PROGRAM:     racoon
## PURPOSE:
## Verify that the racoon daemon correctly negotiates IPsec SAs with remote
## hosts and that when these SAs are added to the kernel's SAD the correct
## audit records are generated.  There is also a test case to verify that the
## SAs are removed correctly but this is a duplicate of the setkey trusted
## program test.  If either the SA is not created or the audit record is
## missing when racoon negotiates a new SA the test fails.  The test procedure
## is as follows:
##  1. Flush any existing SAs from the kernel's SAD
##  2. Attempt to establish a new SA using racoon by talking to a remote
##     test driver over a connection which is configured to require IPsec
##     protection
##  3. Verify the SA was created and the audit trail is correct
##  4. Remove the SA from the kernel's SAD
##  5. Verify the SA was removed and an audit record was generated
## TESTCASE:    negotiate a SA with racoon
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
# get_ipv4_addr - Get the local system's glboal IPv4 address
#
# INPUT
# none
#
# OUTPUT
# Writes the first global IPv4 address on the local system to stdout
#
# DESCRIPTION
# This function queries the local system, through the "ip" command, for a list
# of global IPv4 addresses, it then selects the first address in the list and
# writes it to stdout.
#
function get_ipv4_addr {
    ip -o -f inet addr show scope global | head -n 1 | \
    awk 'BEGIN { FS = "[ \t]*|[ \t\\/]+" } { print $4 }'
}

######################################################################
# functions
######################################################################

#
# ipsec_add - Attempt to negotiate a new IPsec SA using racoon
#
# INPUT
# none
#
# OUTPUT
# none
#
# DESCRIPTION
# This function attempts to negotiate a IPsec SA with a remote node using the
# "racoon" daemon.  The function does this by using a test driver on the remote
# node which the function configures to listen on IPv4/TCP port 5300.  Once the
# remote test driver is waiting for new connections the function tries to
# connect to the remote test driver which triggers a SPD rule in the IPsec
# subsystem which sends a SA "acquire" message to the "racoon" daemon which
# then attempts to negotiate an IPsec SA with the remote host.  If the "racoon"
# deamon is unable to negotiate a SA with the remote host the connection will
# fail.  If this function can not setup the remote test driver or initiate a
# connection to the remote test driver it will fail, calling exit_error() in
# the process.
#
function ipsec_add {
    declare setup_str="recv:ipv4,tcp,5300,0;"
    declare msg_str="Hi Mom!"

    # do the setup
    runcon -t lspp_test_netlabel_t -l SystemLow -- \
	nc -w 1 $ip_dst 5001 <<< $setup_str
    [[ $? != 0 ]] && exit_error "unable to configure the remote system"

    # configure the remote system (try twice to allow for IKE negotiation)
    runcon -t lspp_test_ipsec_t -l SystemLow -- \
	nc -w 1 $ip_dst 5300 <<< $msg_str
    [[ $? == 0 ]] && return
    sleep 2
    runcon -t lspp_test_ipsec_t -l SystemLow -- \
	nc -w 1 $ip_dst 5300 <<< $msg_str
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
    setkey -F
    [[ $? != 0 ]] && exit_error "unable to remove the SA"
}

#
# ipsec_add_verify - Verify that racoon did establish a new SA
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
    setkey -D | grep -q "ah mode=transport" || \
	exit_fail "failed to add the SA"
    augrok --seek=$log_mark type==MAC_IPSEC_ADDSA \
	sec_alg=1 sec_doi=1 sec_obj=staff_u:lspp_test_r:lspp_test_ipsec_t:s0 \
	src=$ip_src dst=$ip_dst protocol=AH res=1 || \
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
    setkey -D | grep -q "ah mode=transport" && \
	exit_fail "failed to remove the SA"
    augrok --seek=$log_mark type==MAC_IPSEC_DELSA \
	sec_alg=1 sec_doi=1 sec_obj=staff_u:lspp_test_r:lspp_test_ipsec_t:s0 \
	src=$ip_src dst=$ip_dst protocol=AH res=1 || \
	exit_fail "missing audit record"
}

######################################################################
# main
######################################################################

set -x

[[ -n $LBLNET_SVR_IPV4 ]] || exit_error
setkey -F || exit_error

# setup the global variables
ip_src=$(get_ipv4_addr)
ip_dst=$LBLNET_SVR_IPV4

# mark the log for augrok later
log_mark=$(stat -c %s $audit_log)

# attempt to negotiate a SA using racoon and verify the results
ipsec_add
ipsec_add_verify

# attempt to remove the SA and verify the results
ipsec_remove
ipsec_remove_verify

# if we made it this far everything is okay
exit_pass
