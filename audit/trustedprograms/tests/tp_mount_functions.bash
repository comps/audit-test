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

source testcase.bash || exit 2

######################################################################
# global variables
######################################################################

unset mnt mntcontext

# this should be different from the default context
prepend_cleanup 'rmdir $mnt'
mnt=$(mktemp -d) || exit_error "mktemp -d failed"
mntcontext=staff_u:object_r:lspp_harness_tmpfs_t:s1:c1

######################################################################
# common functions
######################################################################

function get_fsobj_context {
    ls --scontext -d "$1" | awk '{print $1}'
}

######################################################################
# setup
######################################################################

set -x
