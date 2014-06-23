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
#   along with this pronram;  if not, write to the Free Software
#   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
#
#  FILE: Proc Filesystem Test
#
#  PURPOSE: Test file access on proc filesystem
#
#  HISTORY:
#		3/03 Jerone Young (jeroney@us.ibm.com)
#		9/04 Loulwa Salem (loulwa@us.ibm.com)

USER1=`whoami`
NEW_OWNER="nobody"

FILE_LIST="/proc/kcore /proc/sys/vm/* /proc/sys/kernel/* /proc/sys/fs/*"

EXIT_CODE=0

#-----------------------------------------------------------------------
# FUNCTION:  do_change_file_perm()
#-----------------------------------------------------------------------
do_change_file_perm() {

#Try to change Permission Bits on file

for file in $FILE_LIST
do
	echo "TEST: $USER1 will try to change permission bits on $file"

	PERMISSIONS_BEFORE=`ls -ld $file |awk '{print $1}'`
	chmod -R 777 $file &> /dev/null
	PERMISSIONS_AFTER=`ls -ld $file |awk '{print $1}'`

	if [ "$PERMISSIONS_BEFORE" == "$PERMISSIONS_AFTER" ]
		then
			echo "$USER1 could not change file permission bits of $file (PASSED)"
		else
			echo "$USER1 was able to modify permission bits of $file (FAILED)"
			EXIT_CODE=1
	fi
done
}


#-----------------------------------------------------------------------
# FUNCTION: do_change_owner
#-----------------------------------------------------------------------
do_change_owner() {

#Try to change the owner of the file to $TEST_USER

for file in $FILE_LIST
do
	echo "TEST: Try to change owner of $file"
	OWNER_BEFORE=`ls -ld $file |awk '{print $3}'`
	chown -R $NEW_OWNER $file &> /dev/null
	OWNER_AFTER=`ls -ld $file |awk '{print $3}'`

	if [ "$OWNER_BEFORE" == "$OWNER_AFTER" ]
	then
		echo "$USER1 could not change the owner of $file (PASSED)"
	else
		echo "$USER1 was able to change the owner of $file (FAILED)"
		chown -R root $file
		EXIT_CODE=1
	fi

done
}

#-----------------------------------------------------------------------
# FUNCTION: MAIN
#-----------------------------------------------------------------------
do_change_file_perm
do_change_owner

exit $EXIT_CODE
