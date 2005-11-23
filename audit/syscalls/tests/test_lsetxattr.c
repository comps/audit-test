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
 **  FILE       : test_lsetxattr.c
 **
 **  PURPOSE    : To test the lsetxattr library call auditing.
 **
 **  DESCRIPTION: The test_lsetxattr() function builds into the
 **  laus_test framework to verify that the Linux Audit System
 **  accurately logs both successful and erroneous execution of the
 **  "lsetxattr" system call.
 **
 **  In the successful case, this function:
 **   1) Creates a temporary file
 **   2) Calls lsetxattr on the temporary file with
 **      name="user.mime_type" and value=XATTR_TEST_VALUE
 **   3) Verifies the success result.
 **
 **  The successful case passes a valid path, name, value, size, and
 **  flag to the  lsetxattr call, thus satisfying the conditions as
 **  given in the man page for lsetxattr for a success result.
 **  
 **  In the erroneous case, this function:
 **   1) Calls lsetxattr with path=NULL
 **   2) Verifies the error result.
 **      
 **  The erroneous case causes an EFAULT as detailed in the man page in
 **  stat(2) by passing an invalid address (NULL) to lsetxattr.
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

int test_lsetxattr(struct audit_data *context, int variation, int success)
{

    int rc = 0;
    int exp_errno = -EACCES;

    char *path = NULL;
    char *name = "user.mime_type";
    char value[sizeof(XATTR_TEST_VALUE)];
    size_t size;
    int flags;

	/**
	 * Do as much setup work as possible right here
	 */
    bzero(value, sizeof(XATTR_TEST_VALUE));
    size = sizeof(XATTR_TEST_VALUE);
    flags = XATTR_CREATE;
    // Set up for success
    context->euid = 0;
    context->egid = 0;
    context->fsuid = 0;
    context->fsgid = 0;
    // Create the target file
    if ((rc = createTempFile(&path, S_IRWXU,
			     context->euid, context->egid)) == -1) {
	fprintf(stderr, "ERROR: Cannot create file %s\n", path);
	goto EXIT;
    }
    strcpy(value, XATTR_TEST_VALUE);
    if (context->success) {
	// Nothing to do for success case
    } else {
	// Set up for error
	context->euid = context->fsuid = helper_uid;
    }

    // Set up audit argument buffer
    if ((rc = auditArg5(context,
			AUDIT_ARG_PATH,
			strlen(path), path,
			AUDIT_ARG_STRING, strlen(name), name,
			AUDIT_ARG_POINTER, size, value,
			AUDIT_ARG_IMMEDIATE, sizeof(size_t), &size,
			AUDIT_ARG_IMMEDIATE, sizeof(int), &flags)) != 0) {
	fprintf(stderr, "Error setting up audit argument buffer\n");
	goto EXIT_CLEANUP;
    }
    // Do pre-system call work
    preSysCall(context);

    // Execute system call
    context->u.syscall.exit =
	syscall(__NR_lsetxattr, path, name, value, size, XATTR_CREATE);

    // Do post-system call work
    postSysCall(context, errno, -1, exp_errno);

EXIT_CLEANUP:
	/**
	 * Do cleanup work here
	 */
    // Clean up
    if ((unlink(path)) == -1) {
	fprintf(stderr, "Error unlinking file %s\n", path);
	goto EXIT;
    }

EXIT:
    if (path)
	free(path);
    fprintf(stderr, "Returning from test\n");
    return rc;
}
