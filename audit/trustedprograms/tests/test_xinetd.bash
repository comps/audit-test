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
## PROGRAM:     xinetd
## PURPOSE:
## Verify that the xinetd daemon correctly runs child servers at the same
## sensitivity label as incoming connections when labeled networking is used
## and the "LABELED" option is set for the service.  This test verifies not
## only that the child service reports that it is running at the correct
## sensitivity label but that the audit records indicate the same sensitivity
## label.  The test procedure is as follows:
##  1. Connect to the test driver at a specific sensitivity label
##  2. Verify the reported sensitivity label matches the specified label and
##     that the audit records are correct
## TESTCASE:    connect to a xinetd controlled server at "s3"
## TESTCASE:    connect to a xinetd controlled server at "s4"
## TESTCASE:    connect to a xinetd controlled server at "s5"
## TESTCASE:    connect to a xinetd controlled server at "s5:c1"
## TESTCASE:    connect to a xinetd controlled server at "s5:c1.c5"

source testcase.bash || exit 2

######################################################################
# global variables
######################################################################

######################################################################
# functions
######################################################################

#
# xinetd_test - Test the labeled networking functionality of xinetd
#
# INPUT
# $1 : MLS sensitivity label
#
# OUTPUT
# none
#
# DESCRIPTION
# This function tests xinetd's ability to spawn child processes/servers with
# the same sensitivity label as the incoming connection.  This function
# connects to a local test driver spawned through xinetd; the test driver
# reports back to the client the sensitivity label at which it is running.
# In addition to comparing the actual test driver's sensitivity label with the
# requested sensitivity label this function also checks the audit log to ensure
# that the test driver was in fact spawned with the correct sensitivity label.
# If either the test driver or the audit records indicates a sensitivity label
# which does not match with what was requested the test fails and calls the
# exit_fail() function to indicate failure.
#
function xinetd_test {
    declare log_mark
    declare subj=$1 rem_subj
    declare cmd_str="getcon:mls;"

    # mark the log for augrok later
    log_mark=$(stat -c %s $audit_log)

    # connect through xinetd
    rem_subj="$(runcon -t lspp_test_netlabel_t -l $subj -- \
	        nc -w 1 127.0.0.1 5001 <<< $cmd_str)"
    [[ $? != 0 ]] && exit_error "unable to connect to localhost"

    # verify label
    [[ "$rem_subj" != "$subj" ]] && exit_fail "labels do not match"

    # verify audit record
    augrok --seek=$log_mark type==SYSCALL syscall=execve success=yes \
	subj=system_u:system_r:lspp_harness_t:$subj || \
	exit_fail "missing audit record"

}

######################################################################
# main
######################################################################

set -x

# setup audit
auditctl -a entry,always -S execve
prepend_cleanup "auditctl -d entry,always -S execve"

# run the test with various labels
for iter in "s3" "s4" "s5" "s5:c1" "s5:c1.c5"; do
    xinetd_test $iter
done

# if we made it this far everything is okay
exit_pass
