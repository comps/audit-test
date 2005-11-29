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
   **  FILE       : test_chown.c
   **
   **  PURPOSE    : To test the chown library call auditing.
   **
   **  DESCRIPTION: The test_chown() function builds into the
   **  laus_test framework to verify that the Linux Audit System
   **  accurately logs both successful and erroneous execution of the
   **  "chown" system call.
   **
   **  In the successful case, this function:
   **   1) Generate a file name and create a 777 test file owned by root
   **   2) Clear the audit trail
   **   3) Execute the "chown" system call to change ownership to test user
   **   4) Tests the results of the system call against the
   **      expected successful return
   **
   **  The successful case creates a test file owned by root and with 777
   **  permssions and attempts to change the ownership of the file.
   **  As the file does exist, and the mode is 777, the chown syscall
   **  should succeed.
   **  
   **  In the erroneous case, this function:
   **   1) Generate a file name and do not create the test file
   **   2) Clear the audit trail
   **   3) Execute the "chown" system call on a file that does not exist
   **   4) Tests the results of the system call against the
   **      expected successful return
   **      
   **  The erroneous case has a non-root user try to chown a file owned by root.
   **  This throws an EPERM errno.
   **
   **  HISTORY    :
   **    06/03 Originated by Dustin Kirkland <k1rkland@us.ibm.com>
   **    03/04 Added exp_errno variable by D. Kent Soper <dksoper@us.ibm.com>
   **
   **********************************************************************/

#include "includes.h"
#include "syscalls.h"

int test_chown(struct audit_data *context, int variation, int success)
{


    int rc = 0;
    int exp_errno = -EPERM;
    char *fileName = NULL;
    int owner;
    int group;


     /**
      * Do as much setup work as possible right here
      */
    // Only root may chown, override test user
    owner = context->euid;
    group = context->egid;
    context->euid = 0;
    context->egid = 0;
    context->fsuid = 0;
    context->fsgid = 0;
    // create file with 700 permissions 
    fileName = init_tempfile(S_IRWXU|S_IRWXG|S_IRWXO, context->euid,
			     context->egid);
    if (!fileName) {
	rc = -1;
	goto EXIT;
    }

    if (!context->success) {
	context->euid = context->fsuid = helper_uid;
    }

    // Set up audit argument buffer
    if ((rc = auditArg3(context,
			AUDIT_ARG_PATH,
			strlen(fileName), fileName,
			AUDIT_ARG_IMMEDIATE, sizeof(owner), &owner,
			AUDIT_ARG_IMMEDIATE, sizeof(group), &group)) != 0) {
	fprintf(stderr, "Error setting up audit argument buffer\n");
	goto EXIT;
    }

    // Do pre-system call work
    if ((rc = preSysCall(context)) != 0) {
	fprintf(stderr, "ERROR: pre-syscall setup failed (%d)\n", rc);
	goto EXIT_CLEANUP;
    }
    // Execute system call
    context->u.syscall.exit = syscall(__NR_chown, fileName, owner, group);

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
