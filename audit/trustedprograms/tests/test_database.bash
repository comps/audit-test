#!/bin/bash
# ==========================================================================
#   (C) Copyright Hewlett-Packard Development Company, L.P., 2005
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
#
#
#  FILE   : test_database.bash
#
#  TEST DESCRIPTION: Test the database functionality.
#
#  HISTORY: 11/2011   originated by T.N Santhosh <santhosh.tn@hp.com>
#
#############################################################################

source testcase.bash || exit 2
source tp_ssh_functions.bash || exit 2
disable_ssh_strong_rng

FILENAME=$1

./$FILENAME &> $FILENAME.log
[ $? -eq 0 ] || exit_fail "Test program failed to execute"

retval=`grep "FAIL" $FILENAME.log | wc -l`
echo "TEST PASSED = " `grep "PASS" $FILENAME.log | wc -l` ", FAILED = " $retval >> $FILENAME.log

#Checking status of retval variable
if [ $retval -gt 0 ]; then
        exit_fail
else
        exit_pass
fi
