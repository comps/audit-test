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
   **  FILE   : test_creat.c
   **
   **  PURPOSE: The test_creat() function builds into the laus_test
   **           framework to verify that the Linux Audit System accurately
   **           logs both successful and erroneous execution of the
   **           "creat" system call.
   **
   **           In the successful case, this function:
   **             1) Generates a unique file name
   **             2) Clears the audit trail
   **             3) Executes the "creat" system call
   **             4) Tests the results of the system call against the
   **                expected successful return
   **
   **           The successful case executes the expected conditions
   **           described by the "creat" system call manpage.  That is,
   **           the creat() function is called using a unique
   **           filename, according to a valid mode, and returns a file 
   **           descriptor to be used for subsequent input/output
   **           operations.
   **
   **            In the erroneous case, this function:
   **
   **            The erroneous case executes the faulty conditions
   **            described by the "EACCES" error under the "creat" system
   **            system call manpage.  That is, the creat() function is
   **            called to try to create a file in /root by a non-root user.
   **
   **           
   **
   **
   **  HISTORY:
   **    05/03 originated by Dan Jones (danjones@us.ibm.com)
   **    06/03 furthered by Dustin Kirkland (k1rkland@us.ibm.com)
   **    03/04 Added exp_errno variable by D. Kent Soper <dksoper@us.ibm.com>
   **    05/05 Updates to suppress compiler warnings by Kimberly D. Simon <kdsimon@us.ibm.com>
   **
   **********************************************************************/

#include "includes.h"
#include "syscalls.h"

int test_creat(struct audit_data *context, int variation, int success)
{
    int rc = 0;
    int exp_errno = -EACCES;
    int mode = S_IRWXO;
#if defined(__powerpc64__) || defined(__x86_64__) || defined(__s390x__)
    int flags = O_WRONLY | O_CREAT | O_TRUNC | O_LARGEFILE;
#else
    int flags = O_WRONLY | O_CREAT | O_TRUNC;
#endif
    char *fileName = NULL;
    //char* fileName2 = NULL;

    // create file to choose a filename
    fileName = init_tempfile(S_IRWXU|S_IRWXG|S_IRWXO, context->euid,
			     context->egid);
    if (!fileName) {
	rc = -1;
	goto EXIT;
    }
    // delete file just created so that the syscall can use the temp name
    if ((rc = unlink(fileName)) != 0) {
	fprintf(stderr, "ERROR: Unable to remove file %s: errno=%i\n", fileName, errno);
	goto EXIT;
    }

    if (!context->success) {
	context->euid = context->fsuid = helper_uid;
	fprintf(stderr, "Free file name\n");
	free(fileName);
	fileName = mysprintf("/root/lausXXXXXX");
	printf("Set temp file name mask: %s\n", fileName);
	close(mkstemp(fileName));
	fprintf(stderr, "Not success case, so changing filename to %s\n", fileName);
    }

    // Set up audit argument buffer
    // The creat() system call is actually an open() system call with the
    //  O_CREAT flag set
    if ((rc = auditArg3(context,
			context->success ? AUDIT_ARG_PATH : AUDIT_ARG_STRING,
			strlen(fileName), fileName, AUDIT_ARG_IMMEDIATE,
			sizeof(flags), &flags, AUDIT_ARG_IMMEDIATE,
			sizeof(mode), &mode)) != 0) {
	fprintf(stderr, "Error setting up audit argument buffer\n");
	goto EXIT;
    }
    // Do pre-system call work
    if ((rc = preSysCall(context)) != 0) {
	fprintf(stderr, "ERROR: pre-syscall setup failed (%d)\n", rc);
	goto EXIT_CLEANUP;
    }
    // Execute system call
    context->u.syscall.exit = syscall(__NR_creat, fileName, mode);

    // Do post-system call work
    if ((rc = postSysCall(context, errno, -1, exp_errno)) != 0) {
	fprintf(stderr, "ERROR: post-syscall setup failed (%d)\n", rc);
	goto EXIT_CLEANUP;
    }


EXIT_CLEANUP:
    // creat cleanup
    // close file is successfully created
    if ((unlink(fileName)) != 0) {
	fprintf(stderr, "ERROR: Unable to remove file %s: errno=%i\n", fileName, errno);
	goto EXIT;
    }

EXIT:
    if (fileName)
	free(fileName);
    return rc;
}
