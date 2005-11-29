/*********************************************************************
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
 **  FILE   : test_open.c
 **
 **  PURPOSE: The test_open() function builds into the laus_test
 **           framework to verify that the Linux Audit System accurately
 **           logs both successful and erroneous execution of the
 **           "open" system call.
 **
 **           In the successful case, this function:
 **             1) Creates the temporary file
 **             2) Executes the "open" system call
 **             3) Tests the results of the system call against the
 **                expected successful return
 **
 **           The successful case executes the expected conditions
 **           described by the "open" system call manpage.  That is,
 **           the open() function is called using an existing, readable
 **           filename, according to valid flags, and returns a file 
 **           descriptor to be used for subsequent input/output
 **           operations.
 **
 **            In the erroneous case, this function:
 **             1) Creates a temporary file owned by root, perms 700
 **             2) Attempts to open the file as a test user
 **             3) Tests the results of the system call against the 
 **                expected erroneous return 
 **
 **            The erroneous case executes the faulty conditions
 **            described by the "EACCES" error under the "open" system
 **            system call manpage.  That is, the open() function is
 **            called by a user who does not have permission to read the
 **            file and thus the operation fails.
 **
 **           
 **
 **
 **  HISTORY:
 **    05/03 originated by Dan Jones (danjones@us.ibm.com)
 **    06/03 furthered by Dustin Kirkland (k1rkland@us.ibm.com)
 **    03/04 Added exp_errno variable by D. Kent Soper <dksoper@us.ibm.com>
 **    04/04 Changed to cause EACCES error by Dustin Kirkland (k1rkland@us.ibm.com)
 **
 **********************************************************************/

#include "includes.h"
#include "syscalls.h"

int test_open(struct audit_data *context, int variation, int success)
{
    int rc = 0;
    int exp_errno = -EACCES;
    int fd = 0;
#if defined(__x86_64__) || defined(__powerpc64__) || defined(__s390x__)
    int flags = O_RDONLY | O_LARGEFILE;
#else
    int flags = O_RDONLY;
#endif
    int mode = S_IRWXU;		// mode is ignored in this test case
    char *fileName = NULL;

    if (context->success) {
	// Create a file readable by test user if testing success case
	fileName = init_tempfile(mode, context->euid, context->egid);
	if (!fileName) {
	    rc = -1;
	    goto EXIT;
	}
    } else {
	// Create a file not readable by test user if testing failure case
	fileName = init_tempfile(mode, 0, 0);
	if (!fileName) {
	    rc = -1;
	    goto EXIT;
	}
    }

    // Set up audit argument buffer
    if ((rc = auditArg3(context,
			AUDIT_ARG_PATH,
			strlen(fileName),
			fileName,
			AUDIT_ARG_IMMEDIATE, sizeof(flags), &flags,
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
    fd = context->u.syscall.exit = syscall(__NR_open, fileName, flags, mode);

    // Do post-system call work
    if ((rc = postSysCall(context, errno, -1, exp_errno)) != 0) {
	fprintf(stderr, "ERROR: post-syscall setup failed (%d)\n", rc);
	goto EXIT_CLEANUP;
    }



EXIT_CLEANUP:
    // open cleanup
    if (context->success) {
	// close file if successfully opened
	if ((rc = close(fd)) != 0) {
	    fprintf(stderr, "ERROR: Unable to close file %s: errno=%i\n", fileName,
		    errno);
	    goto EXIT;
	}
    }
    if ((rc = unlink(fileName)) != 0) {
	fprintf(stderr, "ERROR: Unable to remove file %s: errno=%i\n", fileName, errno);
	goto EXIT;
    }

EXIT:
    if (fileName)
	free(fileName);
    return rc;
}
