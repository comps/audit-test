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
 **  FILE       : test_adjtimex.c
 **
 **  PURPOSE    : To test the adjtimex library call auditing.
 **
 **  DESCRIPTION: The test_adjtimex() function builds into the
 **  laus_test framework to verify that the Linux Audit System
 **  accurately logs both successful and erroneous execution of the
 **  "adjtimex" system call.
 **
 **  In the successful case, this function:
 **   1) Calls adjtimex() with tbuf.modes=0 (has no effect)
 **   2) Verifies the success result.
 **
 **  The successful case does not modify the timex buffer values, and
 **  so according to the man page, the call will always succeed.
 **  
 **  In the erroneous case, this function:
 **   1) Calls adjtimex() with tbuf.modes != 0 as the test user
 **   2) Verifies the error result.
 **      
 **  In the erroneous case, we call adjtimex as the test user.
 **  According to the man page, if tbuf.mode is non-zero and the user
 **  is not the superuser, then we will get an error result from the
 **  system call.
 **
 **  HISTORY    :
 **    06/03 Originated by Michael A. Halcrow <mike@halcrow.us>
 **    03/04 Added exp_errno variable by D. Kent Soper <dksoper@us.ibm.com>
 **
 **********************************************************************/

#include "includes.h"
#include "syscalls.h"
#include <sys/timex.h>

int test_adjtimex(struct audit_data *context)
{
    int rc = 0;
    int exp_errno = EPERM;

    struct timex tbuf;

    memset((char *)&tbuf, '\0', sizeof(tbuf));

	/**
	 * Do as much setup work as possible right here
	 */
    // Make sure that we don't get an EPERM in either case
    if (context->success) {
	// Set up for success
	//    context->euid = 0;
	//    context->egid = 0;
	tbuf.modes = 0;
    } else {
	// Set up for error
	tbuf.modes = ADJ_STATUS;
	// EPERM should hit before EINVAL, but just in case...
	tbuf.status = 42;
    }

    // Set up audit argument buffer
    if ((rc = auditArg1(context,
			AUDIT_ARG_POINTER, sizeof(struct timex),
			&tbuf)) != 0) {
	printf1("Error setting up audit argument buffer\n");
	goto EXIT;
    }
    // Do pre-system call work
    if ((rc = preSysCall(context)) != 0) {
	printf1("ERROR: pre-syscall setup failed (%d)\n", rc);
	goto EXIT_CLEANUP;
    }
    // Execute system call
    context->u.syscall.exit = syscall(__NR_adjtimex, &tbuf);

    // Do post-system call work
    if ((rc = postSysCall(context, errno, -1, exp_errno)) != 0) {
	printf1("ERROR: post-syscall setup failed (%d)\n", rc);
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
    printf5("Returning from test\n");
    return rc;
}
