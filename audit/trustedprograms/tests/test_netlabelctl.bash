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
## PROGRAM:     netlabelctl
## PURPOSE:
## Verify that the netlabelctl command correctly configures the kernel's
## NetLabel subsystem.  The test cases verify that the desired configuration
## changes are successful and that the changes generate the expected audit
## entries in the audit log.  If either the configuration change fails or the
## audit trail is incorrect then the test case fails.  The test procedure is
## as follows:
##  1. Record the state of the unlabeled allow flag and setup a callback to
##     restore the configuration when the test completes
##  2. Add a CIPSO DOI definition, LSM mapping, and enable incoming unlabeled
##     packets
##  3. Verify the configuration changes from step #2 and check for the
##     resulting audit records
##  4. Remove the new CIPSO DOI definition and LSM mapping
##  5. Verify the removal of the new CIPSO DOI definition and LSM mapping while
##     checking to make sure both removals are recorded in the audit log
## TESTCASE:    add a CIPSO DOI definition
## TESTCASE:    remove a CIPSO DOI definition
## TESTCASE:    add a LSM/SELlinux domain mapping
## TESTCASE:    remove a LSM/SELinux domain mapping
## TESTCASE:    turn on the unlabeled packet allow flag

source testcase.bash || exit 2

######################################################################
# global variables
######################################################################

unset nlbl_unlbl_allow
unset log_mark

######################################################################
# functions
######################################################################

#
# netlabel_save - Save the existing NetLabel configuration
#
# INPUT
# none
#
# OUTPUT
# none
#
# DESCRIPTION
# This function saves the state of the NetLabel unlabeled allow flag so that
# the current setting can be restored later after the test has been run.  It
# also registes a restore command with the prepend_cleanup() function so that
# the state is restored automatically when this test exits.
#
function netlabel_save {
    nlbl_unlbl_allow=$(netlabelctl unlbl list)
    prepend_cleanup "netlabelctl unlbl accept $nlbl_unlbl_allow"
}

#
# netlabel_add - Configure the NetLabel subsystem using "netlabelctl"
#
# INPUT
# none
#
# OUTPUT
# none
#
# DESCRIPTION
# This function attempts to configure the kernel NetLabel subsystm by adding a
# new CIPSO DOI definition, a LSM domain mapping, and enabling incoming
# unlabeled packets using the "netlabelctl" command.  If any of these
# operations fail the function calls exit_error() to signify failure.
#
function netlabel_add {
    # add a CIPSO DOI definition
    netlabelctl cipsov4 add pass doi:6000 tags:1 &> /dev/null
    [[ $? != 0 ]] && exit_error "unable to perform the add operation"
    # note: this will remove the LSM mapping below if it exists because once
    #       the DOI definition is removed it no longer has any meaning
    prepend_cleanup "netlabelctl cipsov4 del doi:6000 &> /dev/null"

    # add a LSM/SELinux domain mapping
    netlabelctl map add domain:"foo_t" protocol:cipsov4,6000 &> /dev/null
    [[ $? != 0 ]] && exit_error "unable to perform the add operation"

    # allow unlabeled traffic
    netlabelctl unlbl accept on &> /dev/null
    [[ $? != 0 ]] && exit_error "unable to perform the setup operation"
}

#
# netlabel_remove - Undo the NetLabel configuration using "netlabelctl"
#
# INPUT
# none
#
# OUTPUT
# none
#
# DESCRIPTION
# This function attempts to remove the CIPSO DOI definition and LSM domain
# mapping added by the netlabel_add() function using the "netlabelctl" command.
# If either of these removal operations fail the function calls the
# exit_error() function to signify failure.
#
function netlabel_remove {
    # remove the LSM/SELinux domain mapping
    netlabelctl map del domain:"foo_t" &> /dev/null
    [[ $? != 0 ]] && exit_error "unable to perform the remove operation"

    # remove the CIPSO DOI definition
    netlabelctl cipsov4 del doi:6000 &> /dev/null
    [[ $? != 0 ]] && exit_error "unable to perform the remove operation"
}

#
# netlabel_add_verify - Verify the NetLabel configuration
#
# INPUT
# none
#
# OUTPUT
# none
#
# DESCRIPTION
# This function verifies that the netlabel_add() function was successful in all
# of it's attempts to configure the kernel's NetLabel subsystem.  This
# function checks for both the actual configuration changes as well as audit 
# records for each of the changes.  If either the configuration is not correct
# or an audit record is missing the function calls the exit_fail() function to
# signify failure.
#
function netlabel_add_verify {
    declare doi_cmd=$(netlabelctl cipsov4 list doi:6000)
    declare map_cmd=$(netlabelctl map list domain:"foo_t")
    declare unlbl_cmd=$(netlabelctl unlbl list)

    # check the CIPSO DOI definition
    [[ "$doi_cmd" == "tags:1" ]] || exit_fail "failed to configure NetLabel"
    augrok --seek=$log_mark type==MAC_CIPSOV4_ADD \
	cipso_doi=6000 cipso_type=pass res=1 || \
	exit_fail "missing audit record"

    # check the LSM/SELinux domain mapping
    grep -q 'domain:"foo_t",CIPSOv4,6000' <<< $map_cmd || \
	exit_fail "failed to configure NetLabel"
    augrok --seek=$log_mark type==MAC_MAP_ADD \
	nlbl_domain=foo_t nlbl_protocol=cipsov4 cipso_doi=6000 res=1 || \
	exit_fail "missing audit record"

    # check the unlabeled traffic flag
    [[ "$unlbl_cmd" == "on" ]] || exit_fail "failed to configure NetLabel"
    augrok --seek=$log_mark type==MAC_UNLBL_ALLOW unlbl_accept=1 || \
	exit_fail "missing audit record"
}

#
# netlabel_remove_verify - Verify the NetLabel configuration
#
# INPUT
# none
#
# OUTPUT
# none
#
# DESCRIPTION
# This function verifies that the netlabel_remove() function was successful in
# removing the CIPSO DOI definition and LSM domain mapping.  This function
# checks both for the correct NetLabel configuration as well as audit records
# for each of the changes.  If either the configuration is not correct or an
# audit record is missing the function calls the exit_fail() function to
# signify failure.
#
function netlabel_remove_verify {
    declare doi_cmd=$(netlabelctl cipsov4 list)
    declare map_cmd=$(netlabelctl map list)

    # check the LSM/SELinux domain mapping
    grep -q 'domain:"foo_t"' <<< $map_cmd && \
	exit_fail "failed to configure NetLabel"
    augrok --seek=$log_mark type==MAC_MAP_DEL nlbl_domain=foo_t res=1 || \
	exit_fail "missing audit record"

    # check the CIPSO DOI definition
    grep -q '6000,PASS_THROUGH' <<< $doi_cmd && \
	exit_fail "failed to configure NetLabel"
    augrok --seek=$log_mark type==MAC_CIPSOV4_DEL cipso_doi=6000 res=1 || \
	exit_fail "missing audit record"
}

######################################################################
# main
######################################################################

set -x

# mark the log for augrok later
log_mark=$(stat -c %s $audit_log)

# store the present configuration and setup a callback to restore it later
netlabel_save

# attempt to [re]configure the NetLabel subsystem using netlabelctl and verify
# the result
netlabel_add
netlabel_add_verify

# attempt to remove the configuration and verify the result
netlabel_remove
netlabel_remove_verify

# if we made it this far everything is okay
exit_pass
