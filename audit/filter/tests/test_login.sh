#!/bin/bash -x
# =============================================================================
# (c) Copyright Hewlett-Packard Development Company, L.P., 2005
# Written by Matt Anderson <mra@hp.com>
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

export RHOST="localhost"
# TEST_USER and TEST_USER_PASSWD are set in run.bash startup()

# save the date
BEGIN=`date +"%D %T"`

# attempt to login
$(dirname $0)/ssh01_s1 || exit $?

# look for the LOGIN record in the audit log
ausearch -m LOGIN -ts $BEGIN || exit $? 

# tests passed
exit 0
