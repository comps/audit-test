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
 **  FILE       : test_fchown32.c
 **
 **  PURPOSE    : To test the fchown32 library call auditing.
 **
 **  DESCRIPTION: The test_fchown32() function builds into the
 **  laus_test framework to verify that the Linux Audit System
 **  accurately logs both successful and erroneous execution of the
 **  "fchown32" system call.
 **
 **  In the successful case, this function:
 **   1) Generate a file name and create a 777 test file owned by root
 **   2) Clear the audit trail
 **   3) Execute the "fchown32" system call to change ownership to test user
 **   4) Tests the results of the system call against the
 **      expected successful return
 **
 **  The successful case creates a test file owned by root and with 777
 **  permssions and attempts to change the ownership of the file.
 **  As the file does exist, and the mode is 777, the fchown32 syscall
 **  should succeed.
 **  
 **  In the erroneous case, this function:
 **   1) Generate a file name and do not create the test file
 **   2) Clear the audit trail
 **   3) Execute the "fchown32" system call on a file that does not exist
 **   4) Tests the results of the system call against the
 **      expected successful return
 **      
 **  The erroneous case attempts to change the owner of a file that does
 **  not exist.  This tests the system calls handling of the error
 **  described in the manpage and label with the EBADF errno.
 **
 **  HISTORY    :
 **    06/03 Originated by Dustin Kirkland <k1rkland@us.ibm.com>
 **    03/04 Added exp_errno variable by D. Kent Soper <dksoper@us.ibm.com>
 **
 **********************************************************************/

#include "includes.h"
#include "syscalls.h"

int test_fchown32(struct audit_data *context, int variation, int success)
{
    int rc = 0;
#if !defined(__powerpc__) && !defined (__x86_64__) && !defined(__s390x__) && !defined(__ia64__)
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
    // Generate unique filename
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
	goto EXIT;
    }
    // Do pre-system call work
    if ((rc = preSysCall(context)) != 0) {
	fprintf(stderr, "ERROR: pre-syscall setup failed (%d)\n", rc);
	goto EXIT_CLEANUP;
    }
    // Execute system call
    context->u.syscall.exit = syscall(__NR_fchown32, fd, owner, group);

    // Do post-system call work
    if ((rc = postSysCall(context, errno, -1, exp_errno)) != 0) {
	fprintf(stderr, "ERROR: post-syscall setup failed (%d)\n", rc);
	goto EXIT_CLEANUP;
    }


EXIT_CLEANUP:
  /**
   * Do cleanup work here
   */

    if (close(fd) == -1) {
	fprintf(stderr, "Unable to close file descriptor [%d]: errno = [%i]\n", fd,
		errno);
	goto EXIT;
    }

    if ((unlink(fileName)) != 0) {
	fprintf(stderr, "ERROR: Unable to remove file %s: errno=%i\n", fileName, errno);
	goto EXIT;
    }

EXIT:
    if (fileName)
	free(fileName);
    fprintf(stderr, "Returning from test\n");
#endif
    return rc;
}
