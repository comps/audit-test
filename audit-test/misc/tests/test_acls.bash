#!/bin/bash
#
#********************************************************************************
#	Copyright (C) International Business Machines  Corp., 2000
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
#  FILE: ACLs test
#
#  PURPOSE: Wrapper script to run test_acls.bash script.
#
#  HISTORY:
#               09/11 T.N Santhosh (santhosh.tn@hp.com)
#
##################################################################################

source testcase.bash || exit 2

LOG="run.log"
EXE="acl_file"
ACL_RUN="./acl_run"
ACL_TESTS="getfacl-noacl.test setfacl.test acl_permissions.test acl_misc.test"

./$EXE &> $EXE.$LOG

retval+=`grep -v "FAILED" $EXE.$LOG | grep "FAIL" | wc -l`
echo "TEST PASSED = " `grep "PASS" $EXE.$LOG | wc -l` ", FAILED = " $retval

for file in $ACL_TESTS
 do
	if [ ! -e "$file" ]; then
		echo "$file does not exist."; echo
		continue;
	fi

	$ACL_RUN $file &> $file.$LOG;
	retval2+=`grep -v "FAILED" $file.$LOG | grep "FAIL" | wc -l`
	echo "TEST PASSED = " `grep "PASS" $file.$LOG | wc -l` ", FAILED = " $retval2
done

#Checking status of return values
if [ $(($retval + $retval2)) -gt 0 ]; then
	exit_fail
else
	exit_pass
fi
