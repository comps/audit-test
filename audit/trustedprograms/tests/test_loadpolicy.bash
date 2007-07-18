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
# Verify that an authorized user can load the policy with load_policy,
# generating an audit record.  An authorized user is a linux user 
# that is mapped to staff_u selinux user, which the test harness is.  
# It also requires running as root.

source tp_selinux_functions.bash

######################################################################
# main
######################################################################

# cleanup
cleanup_policy

# configure to cleanup at test exit 
prepend_cleanup cleanup_policy

log_mark=$(stat -c %s $audit_log)

set -x 

# try load_policy as the harness user
	load_policy
[[ $? != 0 ]] && exit_error "load policy failed unexpectedly"
augrok --seek=$log_mark type=MAC_POLICY_LOAD \
	subj=$subj auid=$auid success=yes \
	|| exit_fail "missing POLICY LOAD audit record"

exit_pass
