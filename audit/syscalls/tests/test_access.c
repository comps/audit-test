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
   **  FILE       : test_access.c
   **
   **  PURPOSE    : To test the access  library call auditing.
   **
   **  DESCRIPTION: The test_access() function builds into the
   **  laus_test framework to verify that the Linux Audit System
   **  accurately logs both successful and erroneous execution of the
   **  "access" system call.
   **
   **  In the successful case, this function:
   **   1) Generates a unique filename and creates a file with a
   **      particular mode
   **   2) Clears the audit trail
   **   3) Uses the access system call to check a mode that is allowed
   **   4) Tests the results of the system call against the
   **      expected successful return
   **
   **  The successful case creates a file with mode permissions and
   **  uses the "access" system call to check the permissions on the file.
   **  An existing filename and a valid mode is used, and thus the system
   **  call executes correctly.
   **  
   **  In the erroneous case, this function:
   **   1) Generates a unique filename and creates a file with a
   **      particular mode
   **   2) Clears the audit trail
   **   3) Uses the access system call to check a nonsensical mode
   **   4) Tests the results of the system call against the
   **      expected successful return
   **      
   **  The erroneous case tries to access the file in an invalid mode,
   **  thus generating an EACCES errno.
   **
   **  HISTORY    :
   **    06/03 Originated by Dustin Kirkland <k1rkland@us.ibm.com>
   **    03/04 Added exp_errno variable by D. Kent Soper <dksoper@us.ibm.com>
   **
   **********************************************************************/

#include "includes.h"
#include "syscalls.h"

int test_access(struct audit_data *context)
{
    int rc = 0;
    int exp_errno = -EACCES;
    char *fileName = NULL;
    int mode;

     /**
      * Do as much setup work as possible right here
      */
    // Generate unique filename

    if ((rc = createTempFile(&fileName, S_IRUSR,
			     context->euid, context->egid)) == -1) {
	fprintf(stderr, "ERROR: Cannot create file %s\n", fileName);
	goto EXIT;
    }

    if (context->success) {
	mode = R_OK;
    } else {
	mode = W_OK;
    }

    // Set up audit argument buffer
    if ((rc = auditArg2(context,
			AUDIT_ARG_PATH, strlen(fileName), fileName,
			AUDIT_ARG_IMMEDIATE, sizeof(mode), &mode)) != 0) {
	fprintf(stderr, "Error setting up audit argument buffer\n");
	goto EXIT;
    }
    // Do pre-system call work
    if ((rc = preSysCall(context)) != 0) {
	fprintf(stderr, "ERROR: pre-syscall setup failed (%d)\n", rc);
	goto EXIT_CLEANUP;
    }

    context->u.syscall.exit = syscall(__NR_access, fileName, mode);

    // Do post-system call work
    if ((rc = postSysCall(context, errno, -1, exp_errno)) != 0) {
	fprintf(stderr, "ERROR: post-syscall setup failed (%d)\n", rc);
	goto EXIT_CLEANUP;
    }



EXIT_CLEANUP:
     /**
      * Do cleanup work here
      */
    if ((unlink(fileName)) != 0) {
	fprintf(stderr, "ERROR: Unable to remove file %s: errno=%i\n", fileName, errno);
	goto EXIT;
    }

EXIT:
    if (fileName)
	free(fileName);
    fprintf(stderr, "Returning from test\n");
    return rc;
}
