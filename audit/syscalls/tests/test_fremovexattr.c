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
 **  FILE       : test_fremovexattr.c
 **
 **  PURPOSE    : To test the fremovexattr library call auditing.
 **
 **  DESCRIPTION: The test_fremovexattr() function builds into the
 **  laus_test framework to verify that the Linux Audit System
 **  accurately logs both successful and erroneous execution of the
 **  "fremovexattr" system call.
 **
 **  In the successful case, this function:
 **   1) Creates a temporary file
 **   2) Opens the temporary file
 **   3) Sets the user.mime_type extended attribute of the newly
 **      opened temporary file to ``text/plain''
 **   4) Calls fremovexattr on the newly added attribute of the
 **      temporary file
 **   5) Verifies the success result.
 **
 **  The successful case passes a valid file descriptor and name to
 **  the fremovexattr call, thus satisfying the conditions as given in
 **  the man page for fremovexattr for a success result.
 **  
 **  In the erroneous case, this function:
 **   1) Creates a temporary file
 **   2) Opens the temporary file
 **   3) Calls fremovexattr on the test attribute of the temporary file
 **   5) Verifies the erroneous result.
 **      
 **  The erroneous case causes an ENOATTR by trying to remove an attribute
 **  which has not been set.
 **
 **  HISTORY    :
 **    07/03 Originated by Michael A. Halcrow <mike@halcrow.us>
 **    07/03 Furthered by Kylene J. Smith <kylene@us.ibm.com>
 **    03/04 Added exp_errno variable by D. Kent Soper <dksoper@us.ibm.com>
 **
 **********************************************************************/

#include "includes.h"
#include "syscalls.h"
#include <attr/xattr.h>

int test_fremovexattr(struct audit_data *context, int variation, int success)
{

    int rc = 0;
    int exp_errno = -ENOATTR;
    size_t size;
    char *path = NULL;

    int filedes;
    char *name = "user.mime_type";

    //Do as much setup work as possible right here
    size = sizeof(XATTR_TEST_VALUE);
    path = init_tempfile(S_IRWXU | S_IRWXG | S_IRWXO, context->euid,
			 context->egid);
    if (!path) {
	rc = -1;
	goto EXIT;
    }

    if (context->success) {	// Set up for success
	// Create the target file
	if ((rc =
	     setxattr(path, name, "test/plain", strlen(XATTR_TEST_VALUE),
		      XATTR_CREATE)) == -1) {
	    fprintf(stderr, "Error setting attribute [%s]: errno=%i\n", name, errno);
	    goto EXIT_CLEANUP_UNLINK;
	}

    } else {			// Set up for error

    }
    if ((filedes = rc = open(path, O_RDWR)) == -1) {
	fprintf(stderr, "Error opening newly created temporary file [%s]: errno=%i\n",
		path, errno);
	goto EXIT_CLEANUP_UNLINK;
    }
    // Set up audit argument buffer
    if ((rc = auditArg2(context,
			AUDIT_ARG_PATH, strlen(path), path,
			AUDIT_ARG_STRING, strlen(name), name)) != 0) {
	fprintf(stderr, "Error setting up audit argument buffer\n");
	goto EXIT_CLEANUP_CLOSE;
    }
    // Do pre-system call work
    preSysCall(context);

    // Execute system call
    context->u.syscall.exit = syscall(__NR_fremovexattr, filedes, name);

    // Do post-system call work
    postSysCall(context, errno, -1, exp_errno);

EXIT_CLEANUP_CLOSE:
    // Clean up from success case setup
    if ((close(filedes)) == -1) {
	fprintf(stderr, "Error close file descriptor %d\n", filedes);
	goto EXIT_CLEANUP_UNLINK;
    }

EXIT_CLEANUP_UNLINK:
    // Clean up from success case setup
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
