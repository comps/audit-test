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
 **  FILE       : test_umount2.c
 **
 **  PURPOSE    : To test the umount2 library call auditing.
 **
 **  DESCRIPTION: The test_umount2() function builds into the
 **  laus_test framework to verify that the Linux Audit System
 **  accurately logs both successful and erroneous execution of the
 **  "umount2" system call.
 **
 **  In the successful case, this function:
 **   1) Run test as root
 **   2) Create temporary target directory
 **   3) To setup mount system call to mount the virtual proc filesystem
 **      at the temporary directory created in step 2
 **   4) Test: umount2 on temporary directory
 **   5) Remove the temporary directory
 **
 **  The successful case mounts the virtual proc filesystem at the
 **  path specified in target.  Since the target directory is 
 **  created just before the mount and the test user is root we
 **  can expect mount() to successfully execute.
 **  
 **  In the erroneous case, this function:
 **   1) Run test as test user
 **   2) Attemp to umount2 good, mounted directory
 **      
 **  The erroneous case invokes an error by attempting to umount2() as
 **  a non-root user, causing an EPERM error.
 **
 **  HISTORY    :
 **    06/03 Originated by Kylene J. Smith <kylene@us.ibm.com>
 **    03/04 Added exp_errno variable by D. Kent Soper <dksoper@us.ibm.com>
 **
 **********************************************************************/

#include "includes.h"
#include "syscalls.h"
#include <sys/mount.h>

int test_umount2(struct audit_data *context)
{
    int rc = 0;
    int exp_errno = EPERM;
    int flags = 0;
    char *target = NULL;

    printf4("Performing test_umount2\n");

    // dynamically create test directory
    if (rc = (createTempDir(&target, S_IRWXU | S_IRWXG | S_IRWXO,
			    context->euid, context->egid)) == -1) {
	printf1("ERROR: Cannot create dir %s\n", target);
	goto EXIT;
    }
    printf5("Genereated target directory %s\n", target);

    if (rc = (mount("none", target, "proc", 0, NULL)) == -1) {
	printf1("ERROR: Cannot mount dir %s\n", target);
	goto EXIT_CLEANUP;
    }

    if (context->success) {
	// must run as root
	context->euid = 0;
	context->egid = 0;
	context->fsuid = 0;
	context->fsgid = 0;
    }
    // Set up audit argument buffer
    if ((rc = auditArg2(context,
			AUDIT_ARG_PATH,
			strlen(target),
			target,
			AUDIT_ARG_IMMEDIATE, sizeof(int), &flags)) != 0) {
	printf1("Error setting up audit argument buffer\n");
	goto EXIT_CLEANUP;

    }
    // Do pre-system call work
    if ((rc = preSysCall(context)) != 0) {
	printf1("ERROR: pre-syscall setup failed (%d)\n", rc);
	goto EXIT_CLEANUP;
    }
    // Execute system call
    context->u.syscall.exit = syscall(__NR_umount2, target, flags);

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
	if (umount(target) == -1) {
	    printf1
		("Error umounting target directory [%s] for deletion.  Errno: %i\n",
		 target, errno);
	}
    }
    if (rmdir(target) == -1) {
	printf1("Error removing target directory %s: errno%i\n", target, errno);
    }
EXIT:
    if (target)
	free(target);
    printf5("Returning from test\n");
    return rc;
}
