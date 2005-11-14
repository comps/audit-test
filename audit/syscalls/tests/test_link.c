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
   **  FILE       : test_link.c
   **
   **  PURPOSE    : To test the link library call auditing.
   **
   **  DESCRIPTION: The test_link_eacces() function builds into the
   **  laus_test framework to verify that the Linux Audit System
   **  accurately logs both successful and erroneous execution of the
   **  "link" system call.
   **
   **  In the successful case, this function:
   **   1) Generates a unique filename and creates a temporary file
   **   2) Generates a unique filename to use as the link name
   **   3) Clears the audit trail
   **   4) Executes the "link" system call
   **   5) Tests the results of the system call against the
   **      expected successful return
   **
   **  The successful case executes the expected conditions described
   **  by the "link" system call manpage.  That is, the link() function
   **  is called with an existing source file and a valid destination
   **  name.
   **  
   **  In the erroneous case, this function:
   **    1) Tries to create a link in /root as a non-root user
   **      
   **  The erroneous case executes the expected conditions described by 
   **  the "link" system call manpage for erroneous input.  The link()
   **  function is called as a non-root user to put a link in the /root
   **  directory.  This should throw an EACCES errno.
   **
   **  HISTORY    :
   **    06/03 Originated by Dustin Kirkland (k1rkland@us.ibm.com)
   **    03/04 Added exp_errno variable by D. Kent Soper <dksoper@us.ibm.com>
   **
   **********************************************************************/

#include "includes.h"
#include "syscalls.h"
#include <libgen.h>

int test_link(struct audit_data *context)
{

    int rc = 0;
    int exp_errno = EACCES;

    char *source = NULL;
    char *destination = NULL;
    char *res = NULL;

    // Set the syscall-specific data
    printf5("Setting u.syscall.sysnum to %d\n", AUDIT_link);
    context->u.syscall.sysnum = AUDIT_link;

    if ((rc = createTempFile(&source, S_IRWXU | S_IRWXG | S_IRWXO,
			     context->euid, context->egid)) == -1) {
	printf1("ERROR: Cannot create file %s\n", source);
	goto EXIT;
    }

    if (context->success) {
	// In success case, create file name to guarantee uniqueness, then delete it
	if ((rc = createTempFile(&destination, S_IRWXU | S_IRWXG |
				 S_IRWXO, context->euid,
				 context->egid)) == -1) {
	    printf1("ERROR: Cannot create file %s\n", destination);
	    goto EXIT;
	}
	if ((rc = unlink(destination)) != 0) {
	    printf1("ERROR: Unable to remove file %s: errno=%i\n",
		    destination, errno);
	    goto EXIT;
	}
    } else {
	// Fail case, so try to create link in /root as non-root user

	/* ignore leading directories on tempfile path */
	destination = strdup("/root/");
	res = realloc(destination, strlen(source));
	if (!res) {
	    printf1("ERROR: Unable to realloc memory\n");
	    goto EXIT_CLEANUP;
	}
	strcat(destination, basename(source));
	context->euid = context->fsuid = helper_uid;
    }

    // Set up audit argument buffer
    if ((rc = auditArg2(context, AUDIT_ARG_PATH, strlen(source), source,
			context->success ? AUDIT_ARG_PATH : AUDIT_ARG_STRING,
			strlen(destination), destination)) != 0) {
	printf1("Error setting up audit argument buffer\n");
	goto EXIT;
    }
    // Do pre-system call work
    if ((rc = preSysCall(context)) != 0) {
	printf1("ERROR: pre-syscall setup failed (%d)\n", rc);
	goto EXIT_CLEANUP;
    }
    // Execute system call
    context->u.syscall.exit = syscall(__NR_link, source, destination);

    // Do post-system call work
    if ((rc = postSysCall(context, errno, -1, exp_errno)) != 0) {
	printf1("ERROR: post-syscall setup failed (%d)\n", rc);
	goto EXIT_CLEANUP;
    }

EXIT_CLEANUP:
    if ((unlink(source)) != 0) {
	printf1("ERROR: Unable to remove file %s: errno=%i\n", source, errno);
	goto EXIT;
    }
    if (context->success) {
	if ((unlink(destination)) != 0) {
	    printf1("ERROR: Unable to remove file %s: errno=%i\n",
		    destination, errno);
	    goto EXIT;
	}
    }

EXIT:
    if (source) {
	free(source);
    }
    if (destination) {
	free(destination);
    }
    printf5("Returning from test\n");

    return rc;
}
