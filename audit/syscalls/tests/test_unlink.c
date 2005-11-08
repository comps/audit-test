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
   **  FILE       : test_unlink.c
   **
   **  PURPOSE    : To test the unlink library call auditing.
   **
   **  DESCRIPTION: The test_unlink() function builds into the
   **  laus_test framework to verify that the Linux Audit System
   **  accurately logs both successful and erroneous execution of the
   **  "unlink" system call.
   **
   **  In the successful case, this function:
   **   1) Generates a unique filename and creates a temporary file
   **   3) Executes the "unlink" system call
   **
   **  The successful case executes the expected conditions described
   **  by the "unlink" system call manpage.  That is, the unlink() function
   **  is called with an existing valid file.
   **  
   **  In the erroneous case, this function:
   **   1) Execute the "unlink" system call on a file that the euid cannot write
   **      
   **  The erroneous case executes the expected conditions described by 
   **  the "unlink" system call manpage for the EPERM errno.
   **
   **  HISTORY    :
   **    06/03 Originated by Dustin Kirkland (k1rkland@us.ibm.com)
   **    03/04 Added exp_errno variable by D. Kent Soper <dksoper@us.ibm.com>
   **
   **********************************************************************/

#include "includes.h"
#include "syscalls.h"

int test_unlink(laus_data *dataPtr)
{


    int rc = 0;
    int exp_errno = EPERM;
    char *fileName = NULL;
    int remove_me = 0;


    // Set the syscall-specific data
    printf5("Setting laus_var_data.syscallData.code to %d\n", AUDIT_unlink);
    dataPtr->laus_var_data.syscallData.code = AUDIT_unlink;

     /**
      * Do as much setup work as possible right here
      */

    // create the file
    if ((rc = createTempFile(&fileName, S_IRWXU,
			     dataPtr->msg_euid, dataPtr->msg_egid)) == -1) {
	printf1("ERROR: Cannot create file %s\n", fileName);
	goto EXIT;
    }

    if (!dataPtr->successCase) {
	dataPtr->msg_euid = dataPtr->msg_ruid = dataPtr->msg_fsuid = helper_uid;
    }
    // Set up audit argument buffer
    if ((rc = auditArg1(dataPtr,
			AUDIT_ARG_PATH, strlen(fileName), fileName)) != 0) {
	printf1("Error setting up audit argument buffer\n");
	remove_me = 1;
	goto EXIT_CLEANUP;
    }
    // Do pre-system call work
    if ((rc = preSysCall(dataPtr)) != 0) {
	printf1("ERROR: pre-syscall setup failed (%d)\n", rc);
	goto EXIT_CLEANUP;
    }
    // Execute system call
    dataPtr->laus_var_data.syscallData.result = syscall(__NR_unlink, fileName);

    // Do post-system call work
    if ((rc = postSysCall(dataPtr, errno, -1, exp_errno)) != 0) {
	printf1("ERROR: post-syscall setup failed (%d)\n", rc);
	goto EXIT_CLEANUP;
    }

EXIT_CLEANUP:
    /*
     * Do cleanup work here
     */
    if (!dataPtr->successCase) {
	if (unlink(fileName) != 0) {
	    printf1("ERROR: Unable to remove file %s: errno=%i\n", fileName,
		    errno);
	    goto EXIT;
	}
    }

EXIT:
    if (fileName)
	free(fileName);
    printf5("Returning from test\n");
    return rc;
}
