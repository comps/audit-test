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
   **  FILE       : test_chown16.c
   **
   **  PURPOSE    : To test the chown16 library call auditing.
   **
   **  DESCRIPTION: The test_chown16() function builds into the
   **  laus_test framework to verify that the Linux Audit System
   **  accurately logs both successful and erroneous execution of the
   **  "chown16" system call.
   **
   **  In the successful case, this function:
   **   1) Generate a file name and create a 777 test file owned by root
   **   2) Clear the audit trail
   **   3) Execute the "chown16" system call to change ownership to test user
   **   4) Tests the results of the system call against the
   **      expected successful return
   **
   **  The successful case creates a test file owned by root and with 777
   **  permssions and attempts to change the ownership of the file.
   **  As the file does exist, and the mode is 777, the chown16 syscall
   **  should succeed.
   **  
   **  In the erroneous case, this function:
   **      
   **  The erroneous case has a non-root user try to chown a file owned by root.
   **  This throws an EPERM errno.
   **
   **
   **  HISTORY    :
   **    06/03 Originated by Dustin Kirkland <k1rkland@us.ibm.com>
   **    03/04 Added exp_errno variable by D. Kent Soper <dksoper@us.ibm.com>
   **
   **********************************************************************/
#if !defined(__PPC) && !defined(__X86_64) && !defined(__IA64)

#include "includes.h"
#include "syscalls.h"

int test_chown16(struct audit_data *context)
{


    int rc = 0;
    int exp_errno = EPERM;
    char *fileName = NULL;
    int owner;
    int group;


    // Set the syscall-specific data
    printf5("Setting u.syscall.sysnum to %d\n", AUDIT_chown);
    context->u.syscall.sysnum = AUDIT_chown;

     /**
      * Do as much setup work as possible right here
      */
    // Only root may chown16, override test user
    owner = context->euid;
    group = context->egid;
    context->euid = 0;
    context->egid = 0;
    context->fsuid = 0;
    context->fsgid = 0;
    // create file with 777 permissions owned by root
    if ((rc = createTempFile(&fileName, S_IRWXU | S_IRWXG | S_IRWXO,
			     context->euid, context->egid)) == -1) {
	printf1("ERROR: Cannot create file %s\n", fileName);
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
	printf1("Error setting up audit argument buffer\n");
	goto EXIT;
    }

    // Do pre-system call work
    if ((rc = preSysCall(context)) != 0) {
	printf1("ERROR: pre-syscall setup failed (%d)\n", rc);
	goto EXIT_CLEANUP;
    }
    // Execute system call
    context->u.syscall.exit = syscall(__NR_chown16, fileName, owner, group);

    // Do post-system call work
    if ((rc = postSysCall(context, errno, -1, exp_errno)) != 0) {
	printf1("ERROR: post-syscall setup failed (%d)\n", rc);
	goto EXIT_CLEANUP;
    }


EXIT_CLEANUP:
     /**
      * Do cleanup work here
      */
    if ((unlink(fileName)) != 0) {
	printf1("ERROR: Unable to remove file %s: errno=%i\n", fileName, errno);
	goto EXIT;
    }

EXIT:
    if (fileName)
	free(fileName);
    printf5("Returning from test\n");
    return rc;
}
#endif
