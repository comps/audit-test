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
###############################################################################
#   Copyright (c) 2011 Red Hat, Inc. All rights reserved.
#
#   AUTHOR: Ondrej Moris <mvadkert@redhat.com>
#
#   This copyrighted material is made available to anyone wishing
#   to use, modify, copy, or redistribute it subject to the terms
#   and conditions of the GNU General Public License version 2.
#
#   This program is distributed in the hope that it will be
#   useful, but WITHOUT ANY WARRANTY; without even the implied
#   warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
#   PURPOSE. See the GNU General Public License for more details.
#
#   You should have received a copy of the GNU General Public
#   License along with this program; if not, write to the Free
#   Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
#   Boston, MA 02110-1301, USA.
###############################################################################
#
#
## PROGRAM:     ip
## PURPOSE:
## Verify that the ip xfrm command correctly adds and removes both SPD and SAD
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
unset ip_src ip_dst sad_entry_spi ctx
unset spd_entry
unset sad_entry
unset spd_entry_details
unset sad_entry_details
unset spd_add_cmd spd_del_cmd
unset sad_add_cmd sad_del_cmd

######################################################################
# helper functions
######################################################################

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
# IPsec subsystem using the "ip xfrm" command.  If either of these operations
# fail the function calls the exit_error() function to signify failure.
#
function ipsec_add {
    # add a SPD entry
    ip xfrm $spd_add_cmd &> /dev/null
    [[ $? != 0 ]] && exit_error "unable to perform the add operation"
    prepend_cleanup "ip xfrm $spd_del_cmd &> /dev/null"

    # add a SAD entry
    ip xfrm $sad_add_cmd &> /dev/null
    [[ $? != 0 ]] && exit_error "unable to perform the add operation"
    prepend_cleanup "ip xfrm $sad_del_cmd &> /dev/null"
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
# kernel IPsec subsystem by the ipsec_add() using the "ip xfrm" command.  If
# either of these removal operations fail the function calls the exit_error()
# function to signify failure.
#
function ipsec_remove {
    # remove the SAD entry
    ip xfrm $sad_del_cmd &> /dev/null
    [[ $? != 0 ]] && exit_error "unable to perform the delete operation"

    # remove the SPD entry
    ip xfrm $spd_del_cmd &> /dev/null
    [[ $? != 0 ]] && exit_error "unable to perform the delete operation"
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

function ipsec_add_verify {
    # check the SPD entry
    ip xfrm policy list | grep -q "src ${ip_src}/32 dst ${ip_dst}/32 proto icmp" || \
	exit_fail "failed to configure IPsec"

    augrok --seek=$log_mark type==MAC_IPSEC_EVENT \
	op=SPD-add sec_alg=1 sec_doi=1 sec_obj=$ctx \
	src=$ip_src dst=$ip_dst res=1 || exit_fail "missing audit record"

    # check the SAD entry
    ip xfrm state list | grep -q "src ${ip_src} dst ${ip_dst}" || \
	exit_fail "failed to configure IPsec"
    ip xfrm state list | grep -q "proto ah spi $sad_entry_spi reqid 0 mode transport" || \
	exit_fail "failed to configure IPsec"

    augrok --seek=$log_mark type==MAC_IPSEC_EVENT \
	sec_alg=1 sec_doi=1 sec_obj=$ctx \
	src=$ip_src dst=$ip_dst spi="$(printf "%d(0x%x)" $sad_entry_spi $sad_entry_spi)" \
	op=SAD-add || exit_fail "missing audit record"
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

function ipsec_remove_verify {
    # check the SAD entry
    ip xfrm state list | grep -q "src ${ip_src} dst ${ip_dst}" && \
	exit_fail "failed to configure IPsec"
    ip xfrm state list | grep -q "proto ah spi $sad_entry_spi reqid 0 mode transport" && \
	exit_fail "failed to configure IPsec"

    augrok --seek=$log_mark type==MAC_IPSEC_EVENT \
	sec_alg=1 sec_doi=1 sec_obj=$ctx \
	src=$ip_src dst=$ip_dst spi="$(printf "%d(0x%x)" $sad_entry_spi $sad_entry_spi)" \
	op=SAD-delete || exit_fail "missing audit record"

    # check the SPD entry
    ip xfrm policy list | grep -q "src ${ip_src}/32 dst ${ip_dst}/32 proto icmp" && \
	exit_fail "failed to configure IPsec"

    augrok --seek=$log_mark type==MAC_IPSEC_EVENT \
	op=SPD-delete sec_alg=1 sec_doi=1 sec_obj=$ctx \
	src=$ip_src dst=$ip_dst res=1 || exit_fail "missing audit record"
}

#
# ipsec_cleanup - Clean up / restore test-related environment
#
# INPUT
# none
#
# OUTPUT
# none

function ipsec_cleanup {
    # flush xfrm state
    ip xfrm state flush
    # reload default xfrm policy
    if [ "$PASSWD" ]; then
         expect -c "
            spawn bash
            expect {
                \"]#\" { send -- \"run_init service ipsec restart\r\" }
            }
            expect {
                -nocase password: { send -- \"$PASSWD\r\"; exp_continue }
                \"]#\" { send -- \"exit\r\" }
            }"
    else
        echo "warning: PASSWD not set, not reloading xfrm policy"
    fi
}
append_cleanup 'ipsec_cleanup'

######################################################################
# main
######################################################################

set -x

[[ -n $LBLNET_SVR_IPV4 ]] || exit_error
ip xfrm state flush || exit_error
ip xfrm policy flush || exit_error

# setup the global variables
ctx=$(secon -RP)
ip_src=$LOCAL_IPV4
ip_dst=$LBLNET_SVR_IPV4
spd_entry="src $ip_src dst $ip_dst proto icmp ctx $ctx dir out"
spd_entry_detail="tmpl proto ah mode transport level required"
spd_add_cmd="policy add $spd_entry $spd_entry_detail"
spd_del_cmd="policy delete $spd_entry"

sad_entry_spi="0x12345678"
sad_entry="src $ip_src dst $ip_dst proto ah spi $sad_entry_spi"
sad_entry_detail="ctx $ctx auth md5 0123456789012345"
sad_add_cmd="state add $sad_entry $sad_entry_detail"
sad_del_cmd="state delete $sad_entry"

# mark the log for augrok later
log_mark=$(stat -c %s $audit_log)

# attempt to [re]configure the IPsec SPD/SAD using ip xfrm and verify the result
ipsec_add
ipsec_add_verify

# attempt to remove the configuration and verify the result
ipsec_remove
ipsec_remove_verify

# if we made it this far everything is okay
exit_pass
