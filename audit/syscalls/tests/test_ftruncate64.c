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
   **  FILE   : test_ftruncate64.c
   **
   **  PURPOSE: The test_ftruncate64() function builds into the laus_test
   **           framework to verify that the Linux Audit System accurately
   **           logs both successful and erroneous execution of the
   **           "ftruncate64" system call.
   **
   **           In the successful case, this function:
   **             1) Creates and opens the temporary file
   **             2) Executes the "ftruncate64" system call with valid length
   **
   **           The successful case executes the expected conditions
   **           described by the "ftruncate64" system call manpage.  That is,
   **           the ftruncate64() function is called using a valid file descriptor
   **           and length.
   **
   **            In the erroneous case, this function:
   **             1) Creates the temporary file
   **             2) Executes the "ftruncate64" system call with an invalid
   **                length parameter
   **
   **            The erroneous case executes the faulty conditions
   **            described by the "EINVAL" error under the "ftruncate64" system
   **            system call manpage.  That is, the ftruncate64() function is
   **            called using a filename and a negative length.
   **
   **
   **  HISTORY:
   **    06/03 originated by Dustin Kirkland (k1rkland@us.ibm.com)
   **    03/04 Added exp_errno variable by D. Kent Soper <dksoper@us.ibm.com>
   **
   **********************************************************************/

#include "includes.h"
#include "syscalls.h"

   /*
    ** execute a ftruncate64 operation
    */
int test_ftruncate64(struct audit_data *context)
{
    int rc = 0;
#if !defined(__powerpc__) && !defined(__x86_64__) && !defined(__s390x__) && !defined(__ia64__)
    int exp_errno = EINVAL;
    long long length;
    char *fileName = NULL;
    int fd;

    // dynamically create temp file name
    if ((rc = createTempFile(&fileName, S_IRWXU | S_IRWXG | S_IRWXO,
			     context->euid, context->egid)) == -1) {
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
	// if testing success case, set valid length
	length = 1;
    } else {
	// else set negative length
	length = -1;
    }

    // Set up audit argument buffer
    if ((rc = auditArg2(context,
			AUDIT_ARG_PATH, strlen(fileName), fileName,
			AUDIT_ARG_IMMEDIATE, sizeof(u_int64_t), &length)
	) != 0) {
	fprintf(stderr, "Error setting up audit argument buffer\n");
	goto EXIT;
    }
    // Do pre-system call work
    if ((rc = preSysCall(context)) != 0) {
	fprintf(stderr, "ERROR: pre-syscall setup failed (%d)\n", rc);
	goto EXIT_CLEANUP;
    }
    // Execute system call
    context->u.syscall.exit = syscall(__NR_ftruncate64, fd, length);

    // Do post-system call work
    if ((rc = postSysCall(context, errno, -1, exp_errno)) != 0) {
	fprintf(stderr, "ERROR: post-syscall setup failed (%d)\n", rc);
	goto EXIT_CLEANUP;
    }


EXIT_CLEANUP:
    // ftruncate64 cleanup
    if ((rc = unlink(fileName)) != 0) {
	fprintf(stderr, "ERROR: Unable to remove file %s: errno=%i\n", fileName, errno);
	goto EXIT;
    }

EXIT:
    if (fileName)
	free(fileName);
#endif
    return rc;
}
