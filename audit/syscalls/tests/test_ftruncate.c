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
 **  FILE   : test_ftruncate.c
 **
 **  PURPOSE: The test_ftruncate() function builds into the laus_test
 **           framework to verify that the Linux Audit System accurately
 **           logs both successful and erroneous execution of the
 **           "ftruncate" system call.
 **
 **           In the successful case, this function:
 **             1) Creates and opens the temporary file
 **             2) Executes the "ftruncate" system call with valid length
 **
 **           The successful case executes the expected conditions
 **           described by the "ftruncate" system call manpage.  That is,
 **           the ftruncate() function is called using a valid file descriptor
 **           and length.
 **
 **            In the erroneous case, this function:
 **             1) Creates the temporary file
 **             2) Executes the "ftruncate" system call with an invalid file
 **		   descriptor.
 **
 **            The erroneous case executes the faulty conditions
 **            described by the "EBADF" error under the "ftruncate" system
 **            system call manpage.  That is, the ftruncate() function is
 **            called using an invalid file descriptor.
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
 ** execute a ftruncate operation
 */
int test_ftruncate(struct audit_data *context)
{


    int rc = 0;
    __laus_int64 exp_errno = EBADF;
    int length = 1;
    char *fileName = NULL;
    int fd;

    // Set the syscall specific data
    context->u.syscall.sysnum = AUDIT_ftruncate;
    // BUGBUG: Need to understand how to set up syscall parameters

    // dynamically create temp file name
    if ((rc = createTempFile(&fileName, S_IRWXU | S_IRWXG | S_IRWXO,
			     context->euid, context->egid)) == -1) {
	printf1("ERROR: Cannot create file %s\n", fileName);
	goto EXIT;
    }

    if ((fd = open(fileName, O_WRONLY)) == -1) {
	printf1("ERROR: Unable to open %s write only: errno=%i\n",
		fileName, errno);
	rc = fd;
	goto EXIT_CLEANUP;
    }

    if (context->success) {
	// dynamically create temp file name
	if ((rc = createTempFile(&fileName, S_IRWXU | S_IRWXG | S_IRWXO,
				 context->euid, context->egid)) == -1) {
	    printf1("ERROR: Cannot create file %s\n", fileName);
	    goto EXIT;
	}
	if ((fd = open(fileName, O_WRONLY)) == -1) {
	    printf1("ERROR: Unable to open %s write only: errno=%i\n",
		    fileName, errno);
	    rc = fd;
	    goto EXIT_CLEANUP;
	}
    } else {
	// else set bad file descriptor
	fd = -1;
    }

    // Set up audit argument buffer
    if ((rc = auditArg2(context,
			context->success ? AUDIT_ARG_PATH : AUDIT_ARG_ERROR,
			context->
			success ? strlen(fileName) : sizeof(__laus_int64),
			context->success ? fileName : (void *)&exp_errno,
			AUDIT_ARG_IMMEDIATE_u, sizeof(length), &length)) != 0) {
	printf1("Error setting up audit argument buffer\n");
	goto EXIT;
    }
    // Do pre-system call work
    if ((rc = preSysCall(context)) != 0) {
	printf1("ERROR: pre-syscall setup failed (%d)\n", rc);
	goto EXIT_CLEANUP;
    }
    // Execute system call
    context->u.syscall.exit = syscall(__NR_ftruncate, fd, length);

    // Do post-system call work
    if ((rc = postSysCall(context, errno, -1, exp_errno)) != 0) {
	printf1("ERROR: post-syscall setup failed (%d)\n", rc);
	goto EXIT_CLEANUP;
    }


EXIT_CLEANUP:
    // ftruncate cleanup
    if (context->success) {
	if ((rc = unlink(fileName)) != 0) {
	    printf1("ERROR: Unable to remove file %s: errno=%i\n",
		    fileName, errno);
	    goto EXIT;
	}
    }

EXIT:
    if (fileName)
	free(fileName);
    return rc;
}
