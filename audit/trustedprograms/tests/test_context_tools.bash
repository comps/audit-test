#!/bin/bash
#
# =============================================================================
# (c) Copyright Hewlett-Packard Development Company, L.P., 2007
#
#   This program is free software;  you can redistribute it and/or modify
#   it under the terms of the GNU General Public License as published by
#   the Free Software Foundation; either version 2 of the License, or
#   (at your option) any later version.
#
#   This program is distributed in the hope that it will be useful,
#   but WITHOUT ANY WARRANTY;  without even the implied warranty of
#   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See
#   the GNU General Public License for more details.
#
#   You should have received a copy of the GNU General Public License
#   along with this program;  if not, write to the Free Software
#   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
# =============================================================================
#
# PURPOSE:
# Verify that the setfiles and restorecon commands initialize and
# restore file contexts according to the definitions in the policy.
# Test chcon as part of the test by using chcon to change the contexts
# after initialization and before restoration.  For the test to pass,
# everything has to work correctly.
#
# The test procedure is:
# 
# - Load a test policy that defines contexts for test files and directories
# - Create the test files and directories as described in the policy
# - Verify the operation of 'setfiles':
#   - Use 'setfiles' to verify that the newly created files don't have
#     the right initial context
#   - Use 'setfiles' to set the initial contexts on the files
#   - Use 'setfiles' to verify that there are no discrepancies with
#     the policy.  (We verify the context of individual files while
#     testing chcon next.)
# - Verify the operation of 'chcon':
#   - From a privileged type (the test harness), use 'chcon' to modify 
#     the contexts of the files and directories and verify the initial 
#     context and the new contexts
#     - Issue an audit rule to audit setxattr and getxattr calls
#     - Use chcon to upgrade, downgrade and change the category of
#       the files and directories
#     - Use the audit records to verify the initial context and the
#       new contexts
#   - Use 'chcon' from an unprivileged type and verify failures.
# - Verify the operation of 'restorecon':
#   - Use 'restorecon' to verify that the test files and directories
#     have the wrong contexts.
#   - Use 'restorecon' to fix the contexts.
#   - Use 'restorecon' to verify that there are no discrepancies.
# 

source tp_context_functions.bash

######################################################################
# main
######################################################################

# cleanup
cleanup_context_policy

# configure to cleanup at test exit 
append_cleanup cleanup_context_policy
append_cleanup rm -rf $testbase
append_cleanup auditctl -D

set -x

# load the policy module that provides the test contexts
load_test_policy policy/test_context.pp

# Initialize test files and directories
init_files

# Use sefiles to check the contexts on the newly created files and directories
# There should be discrepancies written to $tmp1.
setfiles -o $tmp1 -n policy/test_context.fc $testbase
[[ $? != 0 ]] && exit_error "setfiles failed during setup"
read lines therest <<< "$(wc -l $tmp1)"
if [[ $lines != $items ]]; then
	exit_error "setfiles setup failed"
fi

# Use setfiles to set the contexts on the newly created files and directories
# according to the policy.  
setfiles policy/test_context.fc $testbase
[[ $? != 0 ]] && exit_error "setfiles failed"

# Use setfiles to check the contexts on the files and directories
# There should be no discrepancies written to $tmp1.
setfiles -o $tmp1 -n policy/test_context.fc $testbase
[[ $? != 0 ]] && exit_error "setfiles failed"
read lines therest <<< "$(wc -l $tmp1)"
if [[ $lines != 0 ]]; then
	exit_fail "setfiles found descrepancies"
fi

# Verify that a privileged domain (the test harness) can change
# the level of a file.  This user can upgrade and downgrade.
verify_chcon_level SystemLow SystemHigh $testbase/s0_testfile
verify_chcon_level SystemHigh SystemLow $testbase/s15_testfile
verify_chcon_level Secret s2:c0 $testbase/s2_testfile

# And the same with directories
verify_chcon_level SystemLow SystemHigh $testbase/s0_testdir
verify_chcon_level SystemHigh SystemLow $testbase/s15_testdir
verify_chcon_level Secret s2:c0 $testbase/s2_testdir

# Verify that an unprivileged domain can't change the level.
verify_fail_chcon_level SystemHigh SystemLow $testbase/s0_testdir

# Or a compartment
verify_fail_chcon_level s2:c0 s2:c1 $testbase/s2_testdir

# Check the contexts on the test files...should be alot of differences now
# so there should be discrepancies written to $tmp1.
restorecon -n -R -o $tmp1 $testbase
[[ $? != 0 ]] && exit_error "restorecon failed unexpectedly"
read lines therest <<< "$(wc -l $tmp1)"
if [[ $lines != $items ]]; then
	exit_error "restorecon didn't find right number of descrepancies"
fi

# restore the contexts of the test files
restorecon -R $testbase
[[ $? != 0 ]] && exit_error "restorecon failed unexpectedly"

# Check the contexts on the files and directories
# There should be no discrepancies written to $tmp1.
restorecon -n -R -o $tmp1 $testbase
[[ $? != 0 ]] && exit_error "restorecon failed"
read lines therest <<< "$(wc -l $tmp1)"
if [[ $lines != 0 ]]; then
	exit_fail "restorecon found descrepancies"
fi

exit_pass
