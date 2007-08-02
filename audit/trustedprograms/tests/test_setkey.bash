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
## PROGRAM:     setkey
## PURPOSE:
## Verify that the setkey command correctly adds and removes both SPD and SAD
## entries to the kernel's IPsec subsystem.  The test cases verify that both
## the IPsec kernel database operations, SPD/SAD add or remove, are successful
## and that the operations generate the expected audit entries in the audit
## log.  If either the operation fails or the audit trail is incorrect then
## the test case fails.  The test procedure is as follows:
##  1. Add new SPD and SAD entries to the kernel
##  2. Verify the new entries and the corresponding audit records
##  3. Remove the new SPD and SAD entries from the kernel
##  4. Verify the entries are no longer present and that audit records have
##     been generated showing the removal
## TESTCASE:    add a SPD entry
## TESTCASE:    remove a SPD entry
## TESTCASE:    add a SAD entry
## TESTCASE:    remove a SAD entry

source testcase.bash || exit 2

######################################################################
# global variables
######################################################################

unset log_mark
unset ip_src ip_dst ah_spi ctx
unset spd_entry
unset spd_add_cmd spd_del_cmd
unset sad_add_cmd sad_del_cmd

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
# ipsec_add - Add a IPsec SPD and SAD entry to the kernel
#
# INPUT
# none
#
# OUTPUT
# none
#
# DESCRIPTION
# This function attempts to add both a IPsec SPD and SAD entry to the kernel
# IPsec subsystem using the "setkey" command.  If either of these operations
# fail the function calls the exit_error() function to signify failure.
#
function ipsec_add {
    # add a SPD entry
    setkey -c <<< $spd_add_cmd &> /dev/null
    [[ $? != 0 ]] && exit_error "unable to perform the spdadd operation"
    prepend_cleanup "echo '$spd_del_cmd' | setkey -c &> /dev/null"

    # add a SAD entry
    setkey -c <<< $sad_add_cmd &> /dev/null
    [[ $? != 0 ]] && exit_error "unable to perform the add operation"
    prepend_cleanup "echo '$sad_del_cmd' | setkey -c &> /dev/null"
}

#
# ipsec_remove - Remove a IPsec SPD and SAD entry from the kernel
#
# INPUT
# none
#
# OUTPUT
# none
#
# DESCRIPTION
# This function attempts to remove the IPsec SPD and SAD entries added to
# kernel IPsec subsystem by the ipsec_add() using the "setkey" command.  If
# either of these removal operations fail the function calls the exit_error()
# function to signify failure.
#
function ipsec_remove {
    # remove the SAD entry
    setkey -c <<< $sad_del_cmd &> /dev/null
    [[ $? != 0 ]] && exit_error "unable to perform the delete operation"

    # remove the SPD entry
    setkey -c <<< $spd_del_cmd &> /dev/null
    [[ $? != 0 ]] && exit_error "unable to perform the spddelete operation"
}

#
# ipsec_add_verify - Verify the addition of an IPsec SPD and SAD entry
#
# INPUT
# none
#
# OUTPUT
# none
#
# DESCRIPTION
# This function verifies that the ipsec_add() function was successful in adding
# both a new SPD and SAD entry to the kernel IPsec subsystem.  This function
# checks both for the presence of the new SPD and SAD entries as well as audit
# records for each addition.  If either entry or audit record is not found the
# function calls the exit_fail() function to signify failure.
#
function ipsec_add_verify {
    # check the SPD entry
    setkey -DP | grep -q "${ip_src}\[any\] ${ip_dst}\[any\] icmp" || \
	exit_fail "failed to configure IPsec"
    augrok --seek=$log_mark type==MAC_IPSEC_ADDSPD \
	sec_alg=1 sec_doi=1 sec_obj=$ctx \
	src=$ip_src dst=$ip_dst res=1 || exit_fail "missing audit record"

    # check the SAD entry
    setkey -D | grep -q "ah mode=transport spi=$ah_spi" || \
	exit_fail "failed to configure IPsec"
    augrok --seek=$log_mark type==MAC_IPSEC_ADDSA \
	sec_alg=1 sec_doi=1 sec_obj=$ctx \
	src=$ip_src dst=$ip_dst spi="$(printf "%d(0x%x)" $ah_spi $ah_spi)" \
	protocol=AH res=1 || exit_fail "missing audit record"
}

#
# ipsec_remove_verify - Verify the removal of an IPsec SPD and SAD entry
#
# INPUT
# none
#
# OUTPUT
# none
#
# DESCRIPTION
# This function verifies that the ipsec_remove() function was successful in
# removing the SPD and SAD entries from the kernel IPsec subsystem.  This
# function checks both for the presence of the removed SPD and SAD entries as
# well as audit records for each removal.  If either entry is present or the
# audit records are not found the function the calls exit_fail() function to
# signify failure.
#
function ipsec_remove_verify {
    # check the SAD entry
    setkey -D | grep -q "ah mode=transport spi=$ah_spi" && \
	exit_fail "failed to configure IPsec"
    augrok --seek=$log_mark type==MAC_IPSEC_DELSA \
	sec_alg=1 sec_doi=1 sec_obj=$ctx \
	src=$ip_src dst=$ip_dst spi="$(printf "%d(0x%x)" $ah_spi $ah_spi)" \
	protocol=AH res=1 || exit_fail "missing audit record"

    # check the SPD entry
    setkey -DP | grep -q "${ip_src}\[any\] ${ip_dst}\[any\] icmp" && \
	exit_fail "failed to configure IPsec"
    augrok --seek=$log_mark type==MAC_IPSEC_DELSPD \
	sec_alg=1 sec_doi=1 sec_obj=$ctx \
	src=$ip_src dst=$ip_dst res=1 || exit_fail "missing audit record"
}

######################################################################
# main
######################################################################

set -x

[[ -n $LBLNET_SVR_IPV4 ]] || exit_error
setkey -F || exit_error

# setup the global variables
ctx=$(secon -RP)
ip_src=$(get_ipv4_addr)
ip_dst=$LBLNET_SVR_IPV4
ah_spi=123456
spd_entry="$ip_src $ip_dst icmp -ctx 1 1 \"$ctx\" -P out ipsec ah/transport//require"
spd_add_cmd="spdadd $spd_entry;"
spd_del_cmd="spddelete $spd_entry;"
sad_add_cmd="add $ip_src $ip_dst ah $ah_spi -ctx 1 1 \"$ctx\" -A hmac-md5 \"0123456789012345\";"
sad_del_cmd="delete $ip_src $ip_dst ah $ah_spi;"

# mark the log for augrok later
log_mark=$(stat -c %s $audit_log)

# attempt to [re]configure the IPsec SPD/SAD using setkey and verify the result
ipsec_add
ipsec_add_verify

# attempt to remove the configuration and verify the result
ipsec_remove
ipsec_remove_verify

# if we made it this far everything is okay
exit_pass
