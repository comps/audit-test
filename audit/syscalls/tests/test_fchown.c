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
 **  FILE       : test_fchown.c
 **
 **  PURPOSE    : To test the fchown library call auditing.
 **
 **  DESCRIPTION: The test_fchown() function builds into the
 **  laus_test framework to verify that the Linux Audit System
 **  accurately logs both successful and erroneous execution of the
 **  "fchown" system call.
 **
 **  In the successful case, this function:
 **   1) Generate a file name and create a 777 test file owned by root
 **   2) Clear the audit trail
 **   3) Execute the "fchown" system call to change ownership to test user
 **   4) Tests the results of the system call against the
 **      expected successful return
 **
 **  The successful case creates a test file owned by root and with 777
 **  permssions and attempts to change the ownership of the file.
 **  As the file does exist, and the mode is 777, the fchown syscall
 **  should succeed.
 **  
 **  In the erroneous case, this function:
 **
 **  The erroneous case has the executing test user attempt to chown a
 **  file for which that user does not have permission to do so.
 **  This throws an EPERM errno.
 **
 **  HISTORY    :
 **    06/03 Originated by Dustin Kirkland <k1rkland@us.ibm.com>
 **    03/04 Added exp_errno variable by D. Kent Soper <dksoper@us.ibm.com>
 **
 **********************************************************************/

#include "includes.h"
#include "syscalls.h"

int test_fchown(struct audit_data *context, int variation, int success)
{

    int rc = 0;
    int exp_errno = -EPERM;

    char *fileName = NULL;
    int fd = -1;
    int owner;
    int group;

  /**
   * Do as much setup work as possible right here
   */
    owner = context->euid;
    group = context->egid;
    fileName = init_tempfile(S_IRWXU, 0, 0);
    if (!fileName) {
	rc = -1;
	goto EXIT;
    }
    if ((fd = open(fileName, O_WRONLY)) == -1) {
	fprintf(stderr, "ERROR: Unable to open %s write only: errno=%i\n",
		fileName, errno);
	rc = fd;
	goto EXIT_CLEANUP;
    }
    if (context->success) {
	// Only root may chown, override test user
	context->euid = 0;
	context->egid = 0;
	context->fsuid = 0;
	context->fsgid = 0;
    }
    // Set up audit argument buffer
    if ((rc = auditArg3(context,
			AUDIT_ARG_PATH, strlen(fileName), fileName,
			AUDIT_ARG_IMMEDIATE, sizeof(owner), &owner,
			AUDIT_ARG_IMMEDIATE, sizeof(group), &group)) != 0) {
	fprintf(stderr, "Error setting up audit argument buffer\n");
	goto EXIT_CLEANUP;
    }
    // Do pre-system call work
    if ((rc = preSysCall(context)) != 0) {
	fprintf(stderr, "ERROR: pre-syscall setup failed (%d)\n", rc);
	goto EXIT_CLEANUP;
    }
    // Execute system call
    context->u.syscall.exit = syscall(__NR_fchown, fd, owner, group);

    // Do post-system call work
    if ((rc = postSysCall(context, errno, -1, exp_errno)) != 0) {
	fprintf(stderr, "ERROR: post-syscall setup failed (%d)\n", rc);
	goto EXIT_CLEANUP;
    }


EXIT_CLEANUP:
  /**
   * Do cleanup work here
   */
    close(fd);
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
