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
# Verify that semodule correctly detects an incompatible module
# and prevents loading it.  The test uses two policies built with
# checkmodule, one with a neverallow run and the other with a
# conflicting allow rule.  semodule should prevent loading the
# second module.

source tp_selinux_functions.bash || exit 2

######################################################################
# main
######################################################################

# cleanup
cleanup_policy

# configure to cleanup at test exit 
prepend_cleanup cleanup_policy

set -x 

# load the policy with the neverallow rule
load_test_policy policy/test_addread.pp

# Try loading an incompatible policy module, which should fail.
load_test_policy_fail policy/test_addnever.pp
exit_pass
