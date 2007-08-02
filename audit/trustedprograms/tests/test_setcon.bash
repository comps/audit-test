#!/bin/bash
#
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
# PURPOSE:  This test verifies that writes to /proc/<pid>/attr/current, which
# is the selinux context of the process, are protected as appropriate.   This
# test covers the following cases:
# - success: Where a privileged type (the test harness) is able to update its 
#   own context
# - fail_self: Where an unprivileged type (the generic test type) is unable
#   to update its own context
#
# The basic test procedure is:
# - Attempt to write the 'current' file for the selected test case, using
#   runcon to run as the appropriate type.
# - Verify that the "current" context was changed, or not, as expected.
# - In the success case, verify that the lspp_policy audits the change.
#
#   Note:  I'm not checking audit records in the failure case because the
#   operations fail for different reasons with different syscalls.
#   Also note, this is in the trusted programs test directory even though
#   its not testing a trusted program.  Its testing a trusted database
#   instead but there was no obvious home for the test.

source testcase.bash || exit 2

lspp_policy="lspp_policy"
current_con=staff_u:lspp_test_r:lspp_harness_t:s0-s15:c0.c1023
new_pass=staff_u:lspp_test_r:lspp_harness_t:SystemHigh
new_fail_self=staff_u:lspp_test_r:lspp_test_generic_t:Secret
auid=$(</proc/self/loginuid)

######################################################################
# main
######################################################################

# setup
op=$1

set -x

# Verify that the lspp_policy module is loaded. We rely on this for
# the audit records for writing to /proc/self/attr/current.
semodule -l | grep -q $lspp_policy \
	|| exit_error "$lspp_policy not loaded"

# Also verify that we're who we think we are
if [[ $(cat /proc/self/attr/current) != $current_con ]] ; then
	exit_error "not running as $current_con"
fi

case $op in 
	success) 
		runcon_type=lspp_harness_t
		new_context=$new_pass
		;;
	fail_self)	
		runcon_type=lspp_test_generic_t
		new_context=$new_fail_self
		;;
	*) 
		exit_fail "unknown test operation" 
		;;
esac

log_mark=$(stat -c %s $audit_log)

# Now update the context using the helper program
runcon -t $runcon_type -- $TOPDIR/utils/test_setcon $new_context 

case $op:$? in
    success:0)
	augrok -q --seek=$log_mark type==SYSCALL syscall=write \
	    subj=$current_con auid=$auid success=yes \
	    || exit_fail "syscall audit record missing"
	exit_pass ;;
    success:*)
	exit_fail "runcon returned non-zero status" ;;
    fail_self:0)
	exit_fail "runcon returned zero status" ;;
    fail_self:*)
	exit_pass ;;
esac

# should never be here
exit_error "bueller? bueller?"
