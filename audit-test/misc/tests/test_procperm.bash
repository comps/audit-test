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
#  FILE: Proc Filesystem Test
#
#  PURPOSE: Wrapper script to run procpermtest.sh script as user perm_user.
#
#  HISTORY:
#		9/04 Loulwa Salem (loulwa@us.ibm.com)
#		09/11 T.N Santhosh (santhosh.tn@hp.com)
#
#  Note: 	Test originally called procperm.sh is now test_procperm.bash

TEST_USER='perm_user'
TEST_USER_PASSWD='ltP_t3st*_pass'
TEST_USER_ENCRYPTED_PASSWD='$6$mdf9vvfz$2hQcpjsaKz21PUmjoVfLT23XZb/HbFEKmK6GePHj3arBU2cadAmVDcakSU9HgjaI0u.yzx.XAS3hNXZLtuCZ1.'

#-----------------------------------------------------------------------
# FUNCTION:  create_user
#-----------------------------------------------------------------------

function create_user(){
        echo "Creating test user $TEST_USER..."

        #erase user if he may exist , so we can have a clean env
        killall -9 -u $TEST_USER
        userdel -rf $TEST_USER >& /dev/null
	groupdel $TEST_USER >& /dev/null

        useradd -m -p "$TEST_USER_ENCRYPTED_PASSWD" $TEST_USER

        if [ "$?" != 0 ]; then
                echo "Could not add test user $TEST_USER."
                exit 1
        fi

	return 0
}

#-----------------------------------------------------------------------
# FUNCTION:  delete_user
#-----------------------------------------------------------------------

function delete_user(){
        echo "Deleting test user $TEST_USER..."

        killall -9 -u $TEST_USER
        userdel -rf $TEST_USER >& /dev/null

        if [ "$?" != "0" ]; then
                echo "Not able to delete test user $TEST_USER."
                exit 1
        fi

	groupdel $TEST_USER >& /dev/null

	return 0
}

#
# main
#

create_user
/bin/su $TEST_USER -c ./procpermtest.sh
delete_user
