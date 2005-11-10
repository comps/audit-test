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
   **  FILE       : test_rmdir.c
   **
   **  PURPOSE    : To test the rmdir library call auditing.
   **
   **  DESCRIPTION: The test_rmdir() function builds into the
   **  laus_test framework to verify that the Linux Audit System
   **  accurately logs both successful and erroneous execution of the
   **  "rmdir" system call.
   **
   **  In the successful case, this function:
   **   1) Creates the temporary directory
   **   2) Makes the rmdir syscall on the temporary directory
   **
   **  The successful case removes the directory in the specified in the
   **  path.  Since the directory is created immediately before removing
   **  to it, we can expect rmdir() to successfully execute.
   **  
   **  In the erroneous case, this function:
   **   1) Makes the rmdir syscall on a directory as a user who does
   **      not have permission on that directory.
   **      
   **  The erroneous case invokes an error by trying to remove a directory
   **  as a user who does not have permission to do so, causing an EPERM 
   **  errno.
   **
   **  HISTORY    :
   **    06/03 Originated by Michael A. Halcrow <mike@halcrow.us>
   **    06/03 Furthered by Dustin Kirkland <k1rkland@us.ibm.com>
   **    03/04 Added exp_errno variable by D. Kent Soper <dksoper@us.ibm.com>
   **    05/04 Updates to compile warnings by Kimberly D. Simon <kdsimon@us.ibm.com>
   **
   **********************************************************************/

#include "includes.h"
#include "syscalls.h"

int test_rmdir(struct audit_data *context)
{
    int rc = 0;
    int exp_errno = EPERM;

    char *path = NULL;


    // Set the syscall-specific data
    printf5("Setting u.syscall.sysnum to %d\n", AUDIT_rmdir);
    context->u.syscall.sysnum = AUDIT_rmdir;

    // dynamically create test directory
    if ((rc = (createTempDir(&path, S_IRWXU,
			     context->euid, context->egid)) == -1)) {
	printf1("ERROR: Cannot create dir %s\n", path);
	goto EXIT;
    }
    printf5("Generated directory %s\n", path);

    // Set up audit argument buffer
    if ((rc = auditArg1(context, AUDIT_ARG_PATH, strlen(path), path)) != 0) {
	printf1("Error setting up audit argument buffer\n");
	goto EXIT;
    }

    if (!context->success) {
	context->euid = context->fsuid = helper_uid;
    }
    // Do pre-system call work
    if ((rc = preSysCall(context)) != 0) {
	printf1("ERROR: pre-syscall setup failed (%d)\n", rc);
	goto EXIT_CLEANUP;
    }
    // Execute system call
    context->u.syscall.exit = syscall(__NR_rmdir, path);

    // Do post-system call work
    if ((rc = postSysCall(context, errno, -1, exp_errno)) != 0) {
	printf1("ERROR: post-syscall setup failed (%d)\n", rc);
	goto EXIT_CLEANUP;
    }


EXIT_CLEANUP:
     /**
      * Do cleanup work here
      */
    if (!context->success) {
	rmdir(path);
    }

EXIT:
    if (path)
	free(path);
    printf5("Returning from test\n");
    return rc;
}
