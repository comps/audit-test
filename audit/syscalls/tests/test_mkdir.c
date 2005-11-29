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
   **  FILE       : test_mkdir.c
   **
   **  PURPOSE    : To test the mkdir library call auditing.
   **
   **  DESCRIPTION: The test_mkdir() function builds into the
   **  laus_test framework to verify that the Linux Audit System
   **  accurately logs both successful and erroneous execution of the
   **  "mkdir" system call.
   **
   **  In the successful case, this function:
   **   1) Generates a random new temporary directory name
   **   2) Sets the euid to the test user
   **   3) Performs the mkdir system call with the temporary directory
   **      name
   **   4) Sets the euid to the superuser
   **   5) Tests the syscall return value for successful result
   **   6) Removes the temporary directory.
   **
   **  The successful case generates a new random temporary directory
   **  name and calls mkdir() with that name.  According to the man page
   **  (and assuming the mksfile() function call returns a unique,
   **  nonexistent filename), mkdir() should return success.
   **  
   **  The erroneous case, according to the man page for mkdir(), should
   **  cause an EACCES error result, due to the fact that a non-root user
   **  tries to create a directory in /root
   **
   **  HISTORY    :
   **    06/03 Originated by Michael A. Halcrow <mike@halcrow.us>
   **    03/04 Added exp_errno variable by D. Kent Soper <dksoper@us.ibm.com>
   **
   **********************************************************************/

#include "includes.h"
#include "syscalls.h"

int test_mkdir(struct audit_data *context, int variation, int success)
{
    int rc = 0;
    int exp_errno = -EACCES;

    char *path = NULL;
    int mode;

     /**
      * Do as much setup work as possible right here
      */
    // set the mode
    mode = S_IRWXU | S_IRWXG | S_IRWXO;
    if (context->success) {
	// dynamically create temp file, and delete it real quick
	path = init_tempfile(S_IRWXU|S_IRWXG|S_IRWXO, context->euid,
			     context->egid);
	if (!path) {
	    rc = -1;
	    goto EXIT;
	}
	if ((rc = unlink(path)) != 0) {
	    fprintf(stderr, "ERROR: Unable to remove file %s: errno=%i\n", path, errno);
	    goto EXIT;
	}
	fprintf(stderr, "Generated directory name %s\n", path);
    } else {
	path = mysprintf("/root/tmp");
	unlink(path);
	context->euid = context->fsuid = helper_uid;
    }

    if ((rc = auditArg2(context,
			context->success ? AUDIT_ARG_PATH : AUDIT_ARG_STRING,
			strlen(path), path, AUDIT_ARG_IMMEDIATE, sizeof(int),
			&mode)) != 0) {
	fprintf(stderr, "Error setting up audit argument buffer\n");
	goto EXIT;
    }
    // Do pre-system call work
    if ((rc = preSysCall(context)) != 0) {
	fprintf(stderr, "ERROR: pre-syscall setup failed (%d)\n", rc);
	goto EXIT_CLEANUP;
    }
    // Execute system call
    context->u.syscall.exit = syscall(__NR_mkdir, path, mode);

    // Do post-system call work
    if ((rc = postSysCall(context, errno, -1, exp_errno)) != 0) {
	fprintf(stderr, "ERROR: post-syscall setup failed (%d)\n", rc);
	goto EXIT_CLEANUP;
    }


EXIT_CLEANUP:
     /**
      * Do cleanup work here
      */
    if (context->success) {
	if (rmdir(path) == -1) {
	    fprintf(stderr, "Error removing directory %s during cleanup\n", path);
	    goto EXIT;
	}
    }

EXIT:
    if (path)
	free(path);
    fprintf(stderr, "Returning from test\n");
    return rc;
}
