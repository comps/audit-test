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
 **  FILE       : test_clone.c
 **
 **  PURPOSE    : To test the clone library call auditing.
 **
 **  DESCRIPTION: The test_clone() function builds into the
 **  laus_test framework to verify that the Linux Audit System
 **  accurately logs both successful and erroneous execution of the
 **  "clone" system call.
 **
 **  In the successful case, this function:
 **   1) Malloc some memory for the child process stack
 **   2) Preform the syscall(__NR_clone ...  with the VFORK flag so 
 **      the parent waits.
 **   3) Clean up by freeing the child stack.
 **
 **  The successful case does a syscall(__NR_clone ...in the VFORK 
 **  mode.  The parent will wait and then finish the post_syscall 
 **  and clean-up stuff.
 **
 **  In the erroneous case, this function:
 **   1) Preforms the syscall(__NR_clone ... with the VFORK flag and 
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
 **
 **********************************************************************/

#include "syscalls.h"
#include <sched.h>
#include <unistd.h>

int fn(void *x) {
	sleep(1);
	return 0;
}

int test_clone(laus_data* dataPtr) {


	int rc = 0;
	int exp_errno = EPERM;
	int flags = CLONE_VFORK;
	// int flags = CLONE_FS|CLONE_VFORK|CLONE_PARENT|CLONE_SYSVSEM|0x8000068;

	// Set the syscall-specific data
	printf5( "Setting laus_var_data.syscallData.code to %d\n", AUDIT_clone );
	dataPtr->laus_var_data.syscallData.code = AUDIT_clone;

	// Do as much setup work as possible right here
	if( dataPtr->successCase ) {     // Set up for success

	} else {	// Set up for error

		// BUGBUG: For some reason, we are not able to fail the the clone() syscall
		//   in the 2.6 kernel.
//		flags |= CLONE_NEWNS;
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
	char* stack = malloc(65536);
	if ( ( dataPtr->laus_var_data.syscallData.result = clone( fn, (void*)(stack+32768), flags, NULL ) ) == 0 )
	{
		//In child
		_exit(0);	    
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

