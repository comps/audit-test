#!/bin/sh
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
#  FILE: Permission Test
#
#  PURPOSE: Script to run test_ipc_permission.bash script as user permtest3.
#
#  HISTORY:
#               08/11 T.N Santhosh (santhosh.tn@hp.com)
#


source testcase.bash || exit 2

#be verbose
set -x

FILENAME=$1
TEST_USER2="permtest3"

#-----------------------------------------------------------------------
# FUNCTION:  create_user
#-----------------------------------------------------------------------

function create_user(){
        echo "Creating test user $TEST_USER2..."
        if egrep "^$TEST_USER2" /etc/passwd; then
                userdel $TEST_USER2 >& /dev/null
                [ -d "home/$TEST_USER2" ] && rm -rf /home/$TEST_USER2
                sleep 1
        fi

        useradd -g $TEST_USER2 $TEST_USER2
        if [ "$?" != "0" ]; then
                echo "Could not add test user $TEST_USER2."
                exit 1
        fi
}

#-----------------------------------------------------------------------
# FUNCTION:  create_group
#-----------------------------------------------------------------------

function create_group(){
        echo "Creating group $TEST_USER2..."
        if egrep "^$TEST_USER2" /etc/group; then
                groupdel $TEST_USER2 >& /dev/null
        fi

        groupadd $TEST_USER2
}

#-----------------------------------------------------------------------
# FUNCTION:  delete_user
#-----------------------------------------------------------------------

function delete_user(){
        echo "Deleting test user $TEST_USER2..."
        userdel -r $TEST_USER2 >& /dev/null
        [ -d "/home/$TEST_USER2" ] && rm -rf /home/$TEST_USER2

        sleep 1
        if [ "$?" != "0" ]; then
                echo "Not able to delete test user $TEST_USER2."
                exit 1
        fi
}

#-----------------------------------------------------------------------
# FUNCTION:  delete_group
#-----------------------------------------------------------------------

function delete_group(){
        echo "Deleting group $TEST_USER2..."
        groupdel $TEST_USER2 >& /dev/null
}

#
# main
#

create_group
create_user

./$FILENAME $TEST_USER $TEST_USER2 &> $FILENAME.log
[ $? -eq 0 ] || exit_error "Test program failed to execute"

delete_user
delete_group

retval+=`grep "FAIL" $FILENAME.log | wc -l`
echo "TEST PASSED = " `grep "PASS" $FILENAME.log | wc -l` ", FAILED = " $retval >> $FILENAME.log

#Checking status of retval variable
if [ $retval -gt 0 ]; then
	exit_fail
else
	exit_pass
fi
