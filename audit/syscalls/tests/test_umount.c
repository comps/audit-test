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
   **  FILE       : test_umount.c
   **
   **  PURPOSE    : To test the umount library call auditing.
   **
   **  DESCRIPTION: The test_umount() function builds into the
   **  laus_test framework to verify that the Linux Audit System
   **  accurately logs both successful and erroneous execution of the
   **  "umount" system call.
   **
   **  In the successful case, this function:
   **   1) Run test as root
   **   2) Create temporary target directory
   **   3) To setup mount system call to mount the virtual proc filesystem
   **      at the temporary directory created in step 2
   **   4) Test: umount on temporary directory
   **   5) Remove the temporary directory
   **
   **  The successful case mounts the virtual proc filesystem at the
   **  path specified in target.  Since the target directory is 
   **  created just before the mount and the test user is root we
   **  can expect mount() to successfully execute.
   **  
   **  In the erroneous case, this function:
   **   1) Run test as non-root
   **   2) Attemp to umount good, mounted test directory
   **      
   **  The erroneous case invokes an error by attempting to call umount()
   **  as a non-root user which causes an EPERM.
   **
   **  HISTORY    :
   **    06/03 Originated by Kylene J. Smith <kylene@us.ibm.com>
   **    03/04 Added exp_errno variable by D. Kent Soper <dksoper@us.ibm.com>
   **
   **********************************************************************/
#ifndef __x86_64__

#include "includes.h"
#include "syscalls.h"
#include <sys/mount.h>

int test_umount(struct audit_data *context)
{
    int rc = 0;
    int exp_errno = EPERM;
    char *target = NULL;
    int dummy = 0;

    fprintf(stderr, "Performing test_umount\n");

    // dynamically create test directory
    if (rc = (createTempDir(&target, S_IRWXU | S_IRWXG | S_IRWXO,
			    context->euid, context->egid)) == -1) {
	fprintf(stderr, "ERROR: Cannot create dir %s\n", target);
	goto EXIT;
    }
    fprintf(stderr, "Genereated target directory %s\n", target);

    if (rc = (mount("none", target, "proc", 0, NULL)) == -1) {
	fprintf(stderr, "ERROR: Cannot mount dir %s\n", target);
	goto EXIT_CLEANUP;
    }
    fprintf(stderr, "Mounted source directory \"none\" to "
	    "target directory %s\n", target);

    if (context->success) {
	// must run as root
	context->euid = 0;
	context->egid = 0;
	context->fsuid = 0;
	context->fsgid = 0;
    }
    // Set up audit argument buffer
    if ((rc = auditArg2(context,
			AUDIT_ARG_PATH, strlen(target), target,
			AUDIT_ARG_IMMEDIATE, sizeof(int), &dummy)) != 0) {
	fprintf(stderr, "Error setting up audit argument buffer\n");
	goto EXIT_CLEANUP;
    }
    // Do pre-system call work
    if ((rc = preSysCall(context)) != 0) {
	fprintf(stderr, "ERROR: pre-syscall setup failed (%d)\n", rc);
	goto EXIT_CLEANUP;
    }
    // Execute system call
    context->u.syscall.exit = syscall(__NR_umount, target);

    // Do post-system call work
    if ((rc = postSysCall(context, errno, -1, exp_errno)) != 0) {
	fprintf(stderr, "ERROR: post-syscall setup failed (%d)\n", rc);
	goto EXIT_CLEANUP;
    }

EXIT_CLEANUP:
     /**
      * Do cleanup work here
      */
    if (!context->success) {
	if (umount(target) == -1) {
	    fprintf(stderr, "Error umounting target directory [%s] for deletion."
		    "  Errno: %i\n", target, errno);
	}
    }
    if (rmdir(target) == -1) {
	fprintf(stderr, "Error removing target directory %s: errno%i\n", target, errno);
    }

EXIT:
    if (target) {
	free(target);
    }
    fprintf(stderr, "Returning from test\n");
    return rc;
}
#endif
