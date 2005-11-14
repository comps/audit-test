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
 **  FILE       : test_removexattr.c
 **
 **  PURPOSE    : To test the removexattr library call auditing.
 **
 **  DESCRIPTION: The test_removexattr() function builds into the
 **  laus_test framework to verify that the Linux Audit System
 **  accurately logs both successful and erroneous execution of the
 **  "removexattr" system call.
 **
 **  In the successful case, this function:
 **   1) Creates a temporary file
 **   2) Sets the user.mime_type extended attribute of the newly
 **      created temporary file to text/plain
 **   3) Calls removexattr on the newly added attribute of the
 **      temporary file
 **   4) Verifies the success result.
 **
 **  The successful case passes a valid path and name to the
 **  removexattr call, thus satisfying the conditions as given in the
 **  man page for removexattr for a success result.
 **  
 **  In the erroneous case, this function:
 **   1) Calls removexattr with path=NULL
 **   2) Verifies the error result.
 **      
 **  The erroneous case causes an EFAULT as detailed in the man page in
 **  stat(2) by passing an invalid address (NULL) to removexattr.
 **
 **  HISTORY    :
 **    06/03 Originated by Michael A. Halcrow <mike@halcrow.us>
 **    03/04 Added exp_errno variable by D. Kent Soper <dksoper@us.ibm.com>
 **    04/19 Changed EFAULT to EACCES by Kimberly D. Simon <kdsimon@us.ibm.com>
 **
 **********************************************************************/

#include "includes.h"
#include "syscalls.h"
#include <attr/xattr.h>

int test_removexattr(struct audit_data *context)
{

    int rc = 0;
    int exp_errno = EACCES;
    size_t size;

    char *path = NULL;
    char *name = "user.mime_type";

	/**
	 * Do as much setup work as possible right here
	 */
    size = sizeof(XATTR_TEST_VALUE);
    // Create the target file
    if ((rc = createTempFile(&path, S_IRWXU,
			     context->euid, context->egid)) == -1) {
	printf1("ERROR: Cannot create file %s\n", path);
	goto EXIT;
    }
    if ((rc =
	 setxattr(path, name, "test/plain", strlen(XATTR_TEST_VALUE),
		  XATTR_CREATE)) == -1) {
	printf(" Error setting attribute [%s]: errno=%i\n", name, errno);
	goto EXIT_CLEANUP;
    }
    if (context->success) {	// Set up for success
	// Nothing to do for failure case
    } else {
	// Set up for error
	context->euid = context->fsuid = helper_uid;
    }

    // Set up audit argument buffer
    if ((rc = auditArg2(context,
			AUDIT_ARG_PATH,
			strlen(path), path,
			AUDIT_ARG_STRING, strlen(name), name)) != 0) {
	printf1("Error setting up audit argument buffer\n");
	goto EXIT_CLEANUP;
    }
    // Do pre-system call work
    preSysCall(context);

    // Execute system call
    context->u.syscall.exit = syscall(__NR_removexattr, path, name);

    // Do post-system call work
    postSysCall(context, errno, -1, exp_errno);

EXIT_CLEANUP:
	/**
	 * Do cleanup work here
	 */
    // Clean up
    if ((unlink(path)) == -1) {
	printf1("Error unlinking file %s\n", path);
	goto EXIT;
    }

EXIT:
    if (path)
	free(path);
    printf5("Returning from test\n");
    return rc;
}
