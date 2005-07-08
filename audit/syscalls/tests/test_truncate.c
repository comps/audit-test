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
   **  FILE   : test_truncate.c
   **
   **  PURPOSE: The test_truncate() function builds into the laus_test
   **           framework to verify that the Linux Audit System accurately
   **           logs both successful and erroneous execution of the
   **           "truncate" system call.
   **
   **           In the successful case, this function:
   **             1) Creates the temporary file
   **             2) Executes the "truncate" system call with valid length
   **
   **           The successful case executes the expected conditions
   **           described by the "truncate" system call manpage.  That is,
   **           the truncate() function is called using a valid filename 
   **           and length.
   **
   **            In the erroneous case, this function:
   **             1) Creates the temporary file
   **             2) Executes the "truncate" system call as a user without 
   **                permissions on the file.
   **
   **            The erroneous case executes the faulty conditions
   **            described by the "EACCES" error under the "truncate" system
   **            system call manpage.  That is, the truncate() function is
   **            called using a filename which you don't have permission to.
   **
   **
   **  HISTORY:
   **    06/03 originated by Dustin Kirkland (k1rkland@us.ibm.com)
   **    10/03 furthered by Kylene J. Smith (kylene@us.ibm.com)
   **    03/04 Added exp_errno variable by D. Kent Soper <dksoper@us.ibm.com>
   **    05/04 Updates to suppress compile warnings by Kimberly D. Simon <kdsimon@us.ibm.com>
   **
   **********************************************************************/

#include "includes.h"
#include "syscalls.h"

   /*
    ** execute a truncate operation
    */
int test_truncate(laus_data *dataPtr)
{


    int rc = 0;
    __laus_int64 exp_errno = EACCES;
    off_t length = 1;		//valid value
    //size_t count = 80;     // not needed?
    char *fileName = NULL;


    // Set the syscall specific data
    dataPtr->laus_var_data.syscallData.code = AUDIT_truncate;
    // BUGBUG: Need to understand how to set up syscall parameters

    // dynamically create temp file name
    if ((rc = createTempFile(&fileName, S_IRWXU,
			     dataPtr->msg_euid, dataPtr->msg_egid)) == -1) {
	printf1("ERROR: Cannot create file %s\n", fileName);
	goto EXIT;
    }

    if (!dataPtr->successCase) {
	dataPtr->msg_euid = dataPtr->msg_ruid = dataPtr->msg_fsuid = helper_uid;
    }
    // Set up audit argument buffer
    if ((rc = auditArg2(dataPtr,
			AUDIT_ARG_PATH, strlen(fileName), fileName,
			AUDIT_ARG_IMMEDIATE, sizeof(length), &length)) != 0) {
	printf1("Error setting up audit argument buffer\n");
	goto EXIT;
    }
    // Do pre-system call work
    if ((rc = preSysCall(dataPtr)) != 0) {
	printf1("ERROR: pre-syscall setup failed (%d)\n", rc);
	goto EXIT_CLEANUP;
    }
    // Execute system call
    dataPtr->laus_var_data.syscallData.result =
	syscall(__NR_truncate, fileName, length);

    // Do post-system call work
    if ((rc = postSysCall(dataPtr, errno, -1, exp_errno)) != 0) {
	printf1("ERROR: post-syscall setup failed (%d)\n", rc);
	goto EXIT_CLEANUP;
    }


EXIT_CLEANUP:
    // truncate cleanup
    if ((rc = unlink(fileName)) != 0) {
	printf1("ERROR: Unable to remove file %s: errno=%i\n", fileName, errno);
	goto EXIT;
    }

EXIT:
    if (fileName)
	free(fileName);
    return rc;
}
