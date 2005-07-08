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

#if defined(__MODE_32) || defined(__IX86)
typedef long long __my64;
#else
typedef long __my64;
#endif

#if !defined(__IX86)
// This is needed for accurate processing of headers on 64bit platforms when test
//   is compiled in 31/32bit mode but running on a 64bit kernel (emulation).
//   auditd is running in 64bit mode but compilation of the test suite yields
//   data structures whose sizes are different.
struct rlimit_on_disk {		// edited from /usr/include/bits/resource.h
    /* The current (soft) limit.  */
    __my64 rlim_cur;
    /* The hard limit.  */
    __my64 rlim_max;
};
#else
#define rlimit_on_disk rlimit
#endif

int test_setrlimit(laus_data *dataPtr)
{


    int rc = 0;
    int exp_errno = EINVAL;

    int resource;
    struct rlimit_on_disk rlim;
    struct rlimit syscall_rlim;

    // Set the syscall-specific data
    printf5("Setting laus_var_data.syscallData.code to %d\n", AUDIT_setrlimit);
    dataPtr->laus_var_data.syscallData.code = AUDIT_setrlimit;

	/**
	 * Do as much setup work as possible right here
	 */
    if (dataPtr->successCase) {
	resource = RLIMIT_CPU;

	getrlimit(resource, &syscall_rlim);
	rlim.rlim_cur = (__s32)(syscall_rlim.rlim_cur);
	rlim.rlim_max = (__s32)(syscall_rlim.rlim_max);

	// Set up for success
	// Might include: dataPtr->msg_euid = 0; dataPtr->msg_egid = 0;
    } else {
	// Set up for error
	resource = -1;
    }

    // Set up audit argument buffer
    if ((rc = auditArg2(dataPtr,
			AUDIT_ARG_IMMEDIATE_u, sizeof(int), &resource,
			dataPtr->
			successCase ? AUDIT_ARG_POINTER : AUDIT_ARG_NULL,
			dataPtr->
			successCase ? sizeof(struct rlimit_on_disk) : 0,
			&rlim)) != 0) {
	printf1("Error setting up audit argument buffer\n");
	goto EXIT;
    }
    // Do pre-system call work
    if ((rc = preSysCall(dataPtr)) != 0) {
	printf1("ERROR: pre-syscall setup failed (%d)\n", rc);
	goto EXIT_CLEANUP;
    }
    // Execute system call
    dataPtr->laus_var_data.syscallData.result =
	syscall(__NR_setrlimit, resource, &syscall_rlim);

    // Do post-system call work
    if ((rc = postSysCall(dataPtr, errno, -1, exp_errno)) != 0) {
	printf1("ERROR: post-syscall setup failed (%d)\n", rc);
	goto EXIT_CLEANUP;
    }

EXIT_CLEANUP:
	/**
	 * Do cleanup work here
	 */
    if (dataPtr->successCase) {
	// Clean up from success case setup
    }

EXIT:
    printf5("Returning from test\n");
    return rc;
}
