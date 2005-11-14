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
 **  FILE       : test_settimeofday.c
 **
 **  PURPOSE    : To test the settimeofday library call auditing.
 **
 **  DESCRIPTION: The test_settimeofday() function builds into the
 **  laus_test framework to verify that the Linux Audit System
 **  accurately logs both successful and erroneous execution of the
 **  "settimeofday" system call.
 **
 **  In the successful case, this function must run as root.  It 
 **  creates the necessary time structures and gets the current time to
 **  fill them in.  It then sets the current time to the current time.
 **
 **  The successful case ...
 **   1) Make the user root. 
 **   2) Malloc the time structures
 **   3) Get the current time.
 **   4) Make the settimeofday() call.
 **   5) Clean up.
 **  
 **  In the erroneous case, this function:
 **   1) Makes the settimeofday() call with NULL values
 **      
 **  The erroneous case does not run as root therefore gives the EPERM
 **  error code.  It passes NULL values as parameters (however I could
 **  not make it fail with EINVAL or EFAULT.
 **
 **  HISTORY    :
 **    06/03 Originated by Kylene J. Smith <kylene@us.ibm.com>
 **    03/04 Added exp_errno variable by D. Kent Soper <dksoper@us.ibm.com>
 **    05/04 Updates to suppress compile warnings by Kimberly D. Simon <kdsimon@us.ibm.com>
 **
 **********************************************************************/

#include "includes.h"
#include "syscalls.h"
#include <sys/time.h>

int test_settimeofday(struct audit_data *context)
{
    int rc = 0;
    int exp_errno = EPERM;
    struct timeval *tv = NULL;
    struct timezone *tz = NULL;

    // Do as much setup work as possible right here
    if (context->success) {	// Set up for success
	context->euid = 0;
	context->egid = 0;
	context->fsuid = 0;
	context->fsgid = 0;

	tv = (struct timeval *)malloc(sizeof(struct timeval));
	memset(tv, '\0', sizeof(tv));
	tz = (struct timezone *)malloc(sizeof(struct timezone));
	memset(tz, '\0', sizeof(tz));

	if ((rc = gettimeofday(tv, tz)) < 0) {
	    fprintf(stderr, "Unable to get time of day\n");
	    goto EXIT;
	}
    }
    fprintf(stderr, "time value and time zone structures initialized\n");

    // Set up audit argument buffer
    if ((rc = auditArg2(context, 
	    (context->success ? AUDIT_ARG_POINTER : AUDIT_ARG_NULL), 
	    (context->success ? sizeof(struct timeval) : 0), tv,
	    (context->success ? AUDIT_ARG_POINTER : AUDIT_ARG_NULL),
	    (context->success ? sizeof(struct timezone) : 0), tz)
	) != 0) {
	fprintf(stderr, "Error setting up audit argument buffer\n");
	goto EXIT;
    }
    // Do pre-system call work
    if ((rc = preSysCall(context)) != 0) {
	fprintf(stderr, "ERROR: pre-syscall setup failed (%d)\n", rc);
	goto EXIT;
    }
    // Execute system call
    context->u.syscall.exit = syscall(__NR_settimeofday, tv, tz);

    // Do post-system call work
    if ((rc = postSysCall(context, errno, -1, exp_errno)) != 0) {
	fprintf(stderr, "ERROR: post-syscall failed (%d)\n", rc);
    }

EXIT:
	if (tv) {
		free(tv);
	}
	if (tz) {
		free(tz);
	}

    fprintf(stderr, "Returning from test\n");
    return rc;
}
