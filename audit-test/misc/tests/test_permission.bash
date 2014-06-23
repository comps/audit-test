#!/bin/bash
#
#*********************************************************************
#   Copyright (C) International Business Machines  Corp., 2000
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
#  FILE: Permission Test
#
#  PURPOSE: Wrapper script to run test_permission.bash script as user testuser.
#
#  HISTORY:
#               09/11 T.N Santhosh (santhosh.tn@hp.com)
#


source testcase.bash || exit 2

#be verbose
set -x

FILENAME=$1

#
# main
#

./$FILENAME $TEST_USER &> $FILENAME.log
[ $? -eq 0 ] || exit_fail "Test program failed to execute"

retval+=`grep "FAIL" $FILENAME.log | wc -l`
echo "TEST PASSED = " `grep "PASS" $FILENAME.log | wc -l` ", FAILED = " $retval >> $FILENAME.log

#Checking status of retval variable
if [ $retval -gt 0 ]; then
	exit_fail
else
	exit_pass
fi
