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
    **  FILE       : test_iopl.c
    **
    **  PURPOSE    : To test the iopl library call auditing.
    **
    **  DESCRIPTION: The test_iopl() function builds into the
    **  laus_test framework to verify that the Linux Audit System
    **  accurately logs both successful and erroneous execution of the
    **  "iopl" system call.
    **
    **  In the successful case, this function:
    **   1) Clears the audit trail
    **   2) Makes the iopl syscall with level=1
    **   3) Verifies that iopl returned with a success result
    **   4) Calls the iopl syscall with level=0.
    **
    **  The successful case executes iopl as superuser with parameter
    **  level=1.  According to the man page, we can expect a success
    **  result from iopl given these conditions.
    **  
    **  In the erroneous case, this function:
    **   1) Clears the audit trail
    **   2) Sets the euid to the test user
    **   3) Makes the iopl syscall with level=42
    **   4) Sets the euid to the superuser
    **   5) Verifies that iopl returned with a failure result.
    **      
    **  The erroneous case passes 42 as the level to iopl.  According to
    **  the man page for iopl, this will induce an EINVAL error, since 42
    **  is greater than 3, which is the maximum allowable level.
    **
    **  HISTORY    :
    **    06/03 Originated by Michael A. Halcrow <mike@halcrow.us>
    **    03/04 Added exp_errno variable by D. Kent Soper <dksoper@us.ibm.com>
    **
    **********************************************************************/

#include "includes.h"
#include "syscalls.h"
#include <sys/io.h>

int test_iopl(struct audit_data *context)
{
    int rc = 0;
#ifdef __IX86
    int exp_errno = EINVAL;

    int level;

     /**
      * Do as much setup work as possible right here
      */
    context->euid = 0;
    context->egid = 0;
    context->fsuid = 0;
    context->fsgid = 0;
    if (context->success) {
	level = 1;
    } else {
	level = 42;
    }

    // Set up audit argument buffer
    if ((rc = auditArg1(context,
			AUDIT_ARG_IMMEDIATE, sizeof(int), &level)) != 0) {
	printf1("Error setting up audit argument buffer\n");
	goto EXIT;
    }
    // Do pre-system call work
    if ((rc = preSysCall(context)) != 0) {
	printf1("ERROR: pre-syscall setup failed (%d)\n", rc);
	goto EXIT_CLEANUP;
    }
    // Execute system call
    context->u.syscall.exit = syscall(__NR_iopl, level);

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
	iopl(0);
    }

EXIT:
    printf5("Returning from test\n");
#endif
    return rc;
}
