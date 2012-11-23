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
# Verify that a regular user cannot load an selinux policy.
# In this case "regular" means a non-root linux login that maps to 
# the user_u selinux user, which has no admin privileges.

source tp_selinux_functions.bash || exit 2

######################################################################
# main
######################################################################

# cleanup
cleanup_policy

# configure to cleanup at test exit 
prepend_cleanup cleanup_policy

set -x 

# try loadpolicy as a regular user, expected to fail.
su $TEST_USER -c "
	load_policy
"
[[ $? == 0 ]] && exit_fail "load policy succeeded unexpectedly"

exit_pass
