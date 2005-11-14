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
 **  FILE       : test_setrlimit.c
 **
 **  PURPOSE    : To test the setrlimit library call auditing.
 **
 **  DESCRIPTION: The test_setrlimit() function builds into the
 **  laus_test framework to verify that the Linux Audit System
 **  accurately logs both successful and erroneous execution of the
 **  "setrlimit" system call.
 **
 **  In the successful case, this function:
 **   1) Calls setrlimit on RLIMIT_CPU with cur and max values set to
 **      10
 **   2) Verifies the success condition.
 **
 **  The successful case makes a valid call to setrlimit, according to
 **  the description given on the man page for setrlimit.  The
 **  resource exists and is valid, and the values are within an
 **  acceptable range.
 **  
 **  In the erroneous case, this function:
 **   1) Calls setrlimit on an invalid resource (-1)
 **   2) Verifies the error condition.
 **      
 **  The erroneous case causes an EINVAL error by passing in -1 as the
 **  resource identifier, which is not a valid identifier.
 **
 **  HISTORY    :
 **    06/03 Originated by Michael A. Halcrow <mike@halcrow.us>
 **    03/04 Added exp_errno variable by D. Kent Soper <dksoper@us.ibm.com>
 **
 **********************************************************************/

#include "includes.h"
#include "syscalls.h"
#include <sys/time.h>
#include <sys/resource.h>

int test_setrlimit(struct audit_data *context)
{
    int rc = 0;
    int exp_errno = EINVAL;

    int resource;
    struct rlimit rlim;

	/**
	 * Do as much setup work as possible right here
	 */
    if (context->success) {
	resource = RLIMIT_CPU;

	getrlimit(resource, &rlim);

	// Set up for success
	// Might include: context->euid = 0; context->egid = 0;
    } else {
	// Set up for error
	resource = -1;
    }

    // Set up audit argument buffer
    if ((rc = auditArg2(context,
			AUDIT_ARG_IMMEDIATE_u, sizeof(int), &resource,
			context->success ? AUDIT_ARG_POINTER : AUDIT_ARG_NULL,
			context->success ? sizeof(struct rlimit) : 0, &rlim)
	 ) != 0) {
	fprintf(stderr, "Error setting up audit argument buffer\n");
	goto EXIT;
    }
    // Do pre-system call work
    if ((rc = preSysCall(context)) != 0) {
	fprintf(stderr, "ERROR: pre-syscall setup failed (%d)\n", rc);
	goto EXIT_CLEANUP;
    }
    // Execute system call
    context->u.syscall.exit = syscall(__NR_setrlimit, resource, &rlim);

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
	// Clean up from success case setup
    }

EXIT:
    fprintf(stderr, "Returning from test\n");
    return rc;
}
