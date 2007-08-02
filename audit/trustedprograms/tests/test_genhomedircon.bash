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
# PURPOSE:
# Verify that when the selinux user configuration is changed via semanage,
# that semanage runs genhomedircon to update the context file used by the
# policy, which is /etc/selinux/mls/contexts/files/file_contexts.homedirs.
# Only non-user_u users get listed in file_contexts.homedirs.  Normal users
# inherit a default template.  Abnormal users have it customized for their
# selinux user.
#
# Test procedure:
# - Verify that the test user exist but doesn't have any entries on the
#   file_contexts.homedirs file 
# - Use semanage to map the test user to the staff_u selinux user
# - Verify that there are now entries for this user in the file_contexts.homedirs
#   file and that they're all staff_u
# - Use semanage to reassign the test user to the system_u selinix user
# - Verify the entries in file_contexts.homedirs are now system_u
#
# Note:  This test doesn't test the audit records, those are tested elsewhere.
# However, having the audit records in the log is really handing for debugging if something
# goes wrong.

source tp_selinux_functions.bash || exit 2

#local variables
declare homedirs=/etc/selinux/mls/contexts/files/file_contexts.homedirs

# verification function called with $1=login $2=seuser
function verify_homecontext {
	declare login_lines seuser_lines
	# Look for the test user with the right selinux user contexts...should find some.
	grep $1 $homedirs | grep -q $2 \
		|| exit_fail "verify_homecontext: $1 not $2"

	# Count all the contexts for this user's /home contents, and all the ones
	# that are for this seuser - they should be the same number.
	login_lines=$(grep $1 $homedirs | grep -c /home )
	seuser_lines=$(grep $1 $homedirs | grep /home | grep -c $2)
	if [[ $login_lines != $seuser_lines ]] ; then
		exit_fail "verify_homecontext: $TEST_USER has some non-$2 contexts"
	fi
}

######################################################################
# main
######################################################################

[[ -n $TEST_USER ]] || exit_error "TEST_USER is not set"

# configure to cleanup at test exit 
prepend_cleanup semanage login -d $TEST_USER

set -x

# verify the user has been created 
grep -q $TEST_USER /etc/passwd \
	|| exit_error "test harness failure - test user not created"

# verify there are no contexts for this user in the file_contexts.homedirs file
grep -q $TEST_USER $homedirs \
	&& exit_error "test user already in homedirs"

# assign the test user to the staff_u selinux user
semanage login -a -s staff_u $TEST_USER \
	|| exit_error "semanage failed assigning staff_u"

# now look for the test user with staff_u contexts...should find some.
grep $TEST_USER $homedirs | grep -q staff_u \
	|| exit_fail "$TEST_USER not staff_u"

# Verify that the home context file is correct.
verify_homecontext $TEST_USER staff_u

# Change the user from staff_u to system_u
semanage login -m -s system_u $TEST_USER \
	|| exit_error "semanage failed changing to system_u"

# Verify the home contexts were changed
verify_homecontext $TEST_USER system_u

exit_pass
