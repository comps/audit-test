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
 **  FILE       : test_fchmod.c
 **
 **  PURPOSE    : To test the fchmod library call auditing.
 **
 **  DESCRIPTION: The test_fchmod() function builds into the
 **  laus_test framework to verify that the Linux Audit System
 **  accurately logs both successful and erroneous execution of the
 **  "fchmod" system call.
 **
 **  In the successful case, this function:
 **   1) Generate a file name and create a test file
 **   2) Clear the audit trail
 **   3) Execute the "fchmod" system call to change permissions
 **   4) Tests the results of the system call against the
 **      expected successful return
 **
 **  The successful case creates a test file and attempts to change
 **  the permissions on the file.  As the file does exist, and the mode
 **  is a valid mode, this system call should succeed.
 **  
 **  In the erroneous case, this function:
 **      
 **  The erroneous case attempts to change the mode on a file for which
 **  the executing user does not have permission, thus causing an EPERM
 **  errno.
 **
 **  HISTORY    :
 **    06/03 Originated by Dustin Kirkland <k1rkland@us.ibm.com>
 **    03/04 Added exp_errno variable by D. Kent Soper <dksoper@us.ibm.com>
 **
 **********************************************************************/

#include "includes.h"
#include "syscalls.h"

int test_fchmod(struct audit_data *context)
{

    int rc = 0;
    int exp_errno = -EPERM;

    char *fileName = NULL;
    int fd = -1;
    int mode = S_IRUSR;

  /**
   * Do as much setup work as possible right here
   */
    // Generate unique filename
    if ((rc = createTempFile(&fileName, S_IRWXU, 0, 0)) == -1) {
	fprintf(stderr, "ERROR: Cannot create file %s\n", fileName);
	goto EXIT;
    }
    if ((fd = open(fileName, O_WRONLY)) == -1) {
	fprintf(stderr, "ERROR: Unable to open %s write only: errno=%i\n",
		fileName, errno);
	rc = fd;
	goto EXIT_CLEANUP;
    }
    if (context->success) {
	context->euid = 0;
	context->egid = 0;
	context->fsuid = 0;
	context->fsgid = 0;
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
    // Execute system call
    context->u.syscall.exit = syscall(__NR_fchmod, fd, mode);

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
