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
# Verify that permissions are denied by default and can be
# selectively granted.  The test verifies checkmodule and
# semodule.  The test uses 2 test policy modules built with checkmodule
# during the 'make' of the test suite.  One module only defines types 
# but grants no permissions.  The other grants read permission to one 
# of 2 test domains.  semodule is used to load and update the policies.
#
# The test procedure is:
# - Load the policy that allows no access
# - Attempt to read a file labeled with the test type from both 
#   test domains.  Both should fail.
# - Update the policy to allow read access to one of the domains
# - Attempt to read a file labeled with the test type from both
#   test domains.  One should pass, one should fail.

source tp_selinux_functions.bash || exit 2

######################################################################
# main
######################################################################

# cleanup
cleanup_policy

# configure to cleanup at test exit 
prepend_cleanup cleanup_policy

set -x
# load the policy module that gives no access
load_test_policy policy/test_noaccess.pp

# create and label the test file
touch $tmp1
chcon -t test_open_file_t -l SystemLow $tmp1
[[ $? != 0 ]] && exit_error "chcon failed"

# Try reading the file.  Both domains should fail.
test_open_fail test_open_t $subj_open
test_open_fail test_noopen_t $subj_noopen

# Update the policy to allow one domain to read
update_test_policy policy/test_addread.pp

# Try reading the file.  test_open_t should success
# and test_noopen_t should fail.
test_open_pass test_open_t $subj_open
test_open_fail test_noopen_t $subj_noopen

# Try loading an incompatible policy module, which should fail.
load_test_policy_fail policy/test_addnever.pp
exit_pass
