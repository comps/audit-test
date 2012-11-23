#!/bin/bash
#
#*******************************************************************************
# Copyright (C) International Business Machines  Corp., 2000
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
#  FILE: Test_eal_ld_so_conf.bash
#
#  PURPOSE: Test the access right to the /etc/ld.so.conf file as well as access
#	    rights of all directories contained in /etc/ld.so.conf.
#
#  HISTORY:
#               10/11 T.N Santhosh (santhosh.tn@hp.com)
#

source testcase.bash || exit 2

LOG="ldsoconf.run.log"
FILE="/etc/ld.so.conf"

echo "Check $FILE access" &> $LOG
if [ -e $FILE ]; then
	ls -l $FILE | awk '{
		if ($1 != "-rw-r--r--." || $3 != "root") {
			print $9, "\tFAIL"
		} else {
			print $9, "\tPASS"
		}
	}' >> $LOG
else
	echo "$FILE file does not exist." >> $LOG
fi


LDDIR=`cat /etc/ld.so.conf | awk '{print $2}' | sed -e 's/*.*//'`
LDCONFDIR="/etc/$LDDIR"

echo "Check directories listed in $FPATH" >> $LOG
ls -ld $LDCONFDIR 2>/dev/null | awk '{
       if ($1 != "drwxr-xr-x." || $3 != "root") {
               print $9, "\tFAIL"
       } else {
               print $9, "\tPASS"
       }
}' >> $LOG


retval+=`grep "FAIL" $LOG | wc -l`
echo "TEST PASSED = " `grep "PASS" $LOG | wc -l` " , FAILED = " $retval

#Checking status of return value
if [ $retval -gt 0 ]; then
	exit_fail
else
	exit_pass
fi
