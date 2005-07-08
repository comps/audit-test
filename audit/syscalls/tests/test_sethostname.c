/**********************************************************************
   **   Copyright (C) International Business Machines  Corp., 2003
   **
   **   This program is free software;  you can redistribute it and/or modify
   **   it under the terms of the GNU General Public License as published by
   **   the Free Software Foundation; either version 2 of the License, or
   **   (at your option) any later version.
   **
   **   This program is distributed in the hope that it will be useful,
   **   but WITHOUT ANY WARRANTY;  without even the implied warranty of
   **   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See
   **   the GNU General Public License for more details.
   **
   **   You should have received a copy of the GNU General Public License
   **   along with this program;  if not, write to the Free Software
   **   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
   **
   **
   **
   **  FILE       : test_sethostname.c
   **
   **  PURPOSE    : To test the sethostname library call auditing.
   **
   **  DESCRIPTION: The test_sethostname() function builds into the
   **  laus_test framework to verify that the Linux Audit System
   **  accurately logs both successful and erroneous execution of the
   **  "sethostname" system call.
   **
   **  In the successful case, this function:
   **   1) Gets the current hostname
   **   2) Sets the euid to the superuser
   **   3) Calls sethostname with the current hostname
   **   4) Verifies the success result.
   **
   **  The successful case passes a valid hostname and length to the
   **  sethostname system call as the superuser.  According to the man
   **  page for sethostname, this will result in a success.
   **  
   **  In the erroneous case, this function:
   **   1) Sets the euid to the test user
   **   2) Calls sethostname with a dummy hostname
   **   3) Sets the euid to the superuser
   **   4) Verifies the error result.
   **      
   **  The erroneous case calls sethostname as the test user.  According
   **  to the man page for sethostname, if the caller was not the
   **  superuser, then an EPERM error will result.
   **
   **  HISTORY    :
   **    06/03 Originated by Michael A. Halcrow <mike@halcrow.us>
   **    03/04 Added exp_errno variable by D. Kent Soper <dksoper@us.ibm.com>
   **
   **********************************************************************/

#include "includes.h"
#include "syscalls.h"

int test_sethostname(laus_data *dataPtr)
{

    int rc = 0;
    int exp_errno = EPERM;

    char name[HOST_NAME_MAX];
    size_t len = HOST_NAME_MAX;

    // Set the syscall-specific data
    printf5("Setting laus_var_data.syscallData.code to %d\n",
	    AUDIT_sethostname);
    dataPtr->laus_var_data.syscallData.code = AUDIT_sethostname;

    //Do as much setup work as possible right here
    // Get the current hostname
    if (gethostname(name, len) == -1) {
	printf1("Cannot get current hostname\n");
	goto EXIT;
    }
    printf4("Current hostname = [%s]\n", name);
    len = strlen(name);

    if (dataPtr->successCase) {
	// Set up for success
	dataPtr->msg_euid = 0;
	dataPtr->msg_egid = 0;
	dataPtr->msg_fsuid = 0;
	dataPtr->msg_fsgid = 0;
    } else {
	// Set up for error
	//      strcpy( name, "laus_error_case" );
	//      len = -1;
    }

    // Set up audit argument buffer
    if ((rc = auditArg2(dataPtr,
			dataPtr->
			successCase ? AUDIT_ARG_POINTER : AUDIT_ARG_NULL,
			dataPtr->successCase ? strlen(name) : 0,
			dataPtr->successCase ? &name : NULL,
			AUDIT_ARG_IMMEDIATE, sizeof(size_t), &len)) != 0) {
	printf1("Error setting up audit argument buffer\n");
	goto EXIT;
    }
    // Do pre-system call work
    preSysCall(dataPtr);

    // Execute system call
    dataPtr->laus_var_data.syscallData.result =
	syscall(__NR_sethostname, name, len);

    // Do post-system call work
    if ((rc = postSysCall(dataPtr, errno, -1, exp_errno)) != 0) {
	printf1("ERROR: post-syscall setup failed (%d)\n", rc);
	goto EXIT_CLEANUP;
    }

EXIT_CLEANUP:
    /**
     * Do cleanup work here
     */
    if (dataPtr->successCase) {
	// Clean up from success case setup
    }

EXIT:
    printf5("Returning from test\n");
    return rc;
}
