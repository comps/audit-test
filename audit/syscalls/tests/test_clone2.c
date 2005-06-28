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
 **  FILE       : test_clone2.c
 **
 **  PURPOSE    : To test the clone2 library call auditing.
 **
 **  DESCRIPTION: The test_clone2() function builds into the
 **  laus_test framework to verify that the Linux Audit System
 **  accurately logs both successful and erroneous execution of the
 **  "clone2" system call.
 **
 **  In the successful case, this function:
 **   1) Malloc some memory for the child process stack
 **   2) Preform the syscall(__NR_clone2 ...  with the VFORK flag so 
 **      the parent waits.
 **   3) Clean up by freeing the child stack.
 **
 **  The successful case does a syscall(__NR_clone2 ...in the VFORK 
 **  mode.  The parent will wait and then finish the post_syscall 
 **  and clean-up stuff.
 **
 **  In the erroneous case, this function:
 **   1) Preforms the syscall(__NR_clone2 ... with the VFORK flag and 
 **      the CLONE_PID a NULL value for the stack.  This will only 
 **      fail if user is NOT root.
 **
 **  The erroneous case causes a failure by passing the CLONE_NEWNS flag.
 **  The error is EPERM(1).
 **
 **  HISTORY    :
 **    06/03 Originated by Kylene J. Smith <kylene@us.ibm.com>
 **    03/04 Added exp_errno variable by D. Kent Soper <dksoper@us.ibm.com>
 **    04/04 Modified test to call glibc interface by Kimberly D. Simon <kdsimon@us.ibm.com>
 **    05/05 modified clone() => clone2(): Amy Griffis <amy.griffis@hp.com>
 **
 **********************************************************************/

#include "includes.h"
#include "syscalls.h"
#include <sched.h>

#ifdef __NR_clone2

int test_clone2(laus_data* dataPtr) {


	int rc = 0;
	int exp_errno = EPERM;
	int flags = CLONE_VFORK;
	pid_t pid;

	// Set the syscall-specific data
	// BUGBUG: /usr/include/linux/audit.h should be patched to
	// include AUDIT_clone2
	printf5( "Setting laus_var_data.syscallData.code to %d\n", AUDIT_clone );
	dataPtr->laus_var_data.syscallData.code = AUDIT_clone;

	// Do as much setup work as possible right here
	if( dataPtr->successCase ) {     // Set up for success

	} else {	// Set up for error

		rc = SKIP_TEST_CASE;
		goto EXIT;

	}

	// Set up audit argument buffer
	// Only save off first argument to assist log verification
	if( ( rc = auditArg1( dataPtr, AUDIT_ARG_IMMEDIATE, sizeof(int), &flags) ) != 0 ) {
		printf1( "Error setting up audit argument buffer\n" );
		goto EXIT_CLEANUP;
	}

	// Do pre-system call work
	if ( (rc = preSysCall( dataPtr )) != 0 ) {
		printf1("ERROR: pre-syscall setup failed (%d)\n", rc);
		goto EXIT_CLEANUP;
	}

	// Execute system call--parent waits b/c of CLONE_VFORK flag
	// glibc doesn't export a public clone2 symbol
	pid = syscall( __NR_clone2, flags, NULL );
	switch (pid) {
	    case -1:
		printf1("ERROR: clone2 failed (%d)\n", pid);
		goto EXIT_CLEANUP;
	    case 0:
		//In child
		_exit(0);	    
	    default:
		//In parent
		dataPtr->laus_var_data.syscallData.result = pid;
	}

	// Do post-system call work
	if ( (rc = postSysCall(  dataPtr, errno, -1, exp_errno  )) != 0 ) {
		printf1("ERROR: post-syscall setup failed (%d)\n", rc);
		goto EXIT_CLEANUP;
	}


EXIT_CLEANUP:

EXIT:
	printf5( "Returning from test\n" );
	return rc;
}

#endif /* __NR_clone2 */
