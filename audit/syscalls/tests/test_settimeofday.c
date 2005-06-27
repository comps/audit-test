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

#if !defined(__IX86)
// This is needed for accurate processing of headers on 64bit platforms when test suite
//   is compiled in 31/32bit mode but running on a 64bit kernel (emulation).
//   auditd is running in 64bit mode but compilation of the test suite yields
//   data structures whose sizes are different.
struct timeval_on_disk {   // edited from /usr/include/bits/time.h
	__laus_int64 tv_sec;       /* Seconds.  */
	__laus_int64 tv_usec;      /* Microseconds.  */
};
#else
#define timeval_on_disk timeval
#endif

int test_settimeofday(laus_data* dataPtr) {

	int rc = 0;
	int exp_errno = EPERM;
	struct timeval *tv = NULL;
	struct timeval_on_disk *tv_on_disk = NULL;
	struct timezone *tz = NULL;

	// Set the syscall-specific data
	printf5( "Setting laus_var_data.syscallData.code to %d\n", AUDIT_settimeofday );
	dataPtr->laus_var_data.syscallData.code = AUDIT_settimeofday;

	// Do as much setup work as possible right here
	if( dataPtr->successCase ) { // Set up for success
		dataPtr->msg_euid = 0; 
		dataPtr->msg_egid = 0;
		dataPtr->msg_fsuid = 0;
		dataPtr->msg_fsgid = 0;

		tv = (struct timeval*) malloc( sizeof(struct timeval));
		memset(tv, '\0', sizeof(tv));
		tv_on_disk = (struct timeval_on_disk*) malloc( sizeof(struct timeval_on_disk));
		memset(tv_on_disk, '\0', sizeof(tv_on_disk));
		tz = (struct timezone*) malloc( sizeof(struct timezone));
		memset(tz, '\0', sizeof(tz));

		if ( (rc = gettimeofday(tv, tz)) < 0 ) {
			printf1("Unable to get time of day\n");
			goto EXIT;
		}
		// We have to create the second (almost identical structure) which is the way
		// this data is actually written to the disk.
		// This is need for zSeries compatibility when running in 31bit mode on a 
		// 64bit kernel.
		tv_on_disk->tv_sec = tv->tv_sec;
		//BUGBUG: The audit record appears to contain
		//	nanoseconds instead of microseconds.
		//	aucat/augrep report values correctly.
		tv_on_disk->tv_usec = tv->tv_usec*1000;
	} else { // Set up for error

	}
	printf5("time value and time zone structures initialized\n");

	// Set up audit argument buffer
	if( (rc = auditArg2( dataPtr, 
					( dataPtr->successCase ? AUDIT_ARG_POINTER : AUDIT_ARG_NULL),
					( dataPtr->successCase ? sizeof( struct timeval_on_disk) : 0), 
					tv_on_disk,
					( dataPtr->successCase ? AUDIT_ARG_POINTER : AUDIT_ARG_NULL),
					( dataPtr->successCase ? sizeof( struct timezone) : 0),
					tz 
			   ) 
	    ) != 0 ) {
		printf1( "Error setting up audit argument buffer\n" );
		goto EXIT;
	}

	// Do pre-system call work
	if ( ( rc = preSysCall( dataPtr ) ) != 0 ) {
		printf1("ERROR: pre-syscall setup failed (%d)\n", rc);
		goto EXIT;
	}

	// Execute system call
	dataPtr->laus_var_data.syscallData.result = syscall( __NR_settimeofday, tv, tz );

	// Do post-system call work
	if ( (rc = postSysCall( dataPtr, errno, -1, exp_errno )) != 0 ) {
		printf1("ERROR: post-syscall failed (%d)\n", rc);
	}

	//EXIT_CLEANUP:     // not needed?
	//Do cleanup work here
	if( dataPtr->successCase ) {	// Clean up from success case setup
		free(tv);
		free(tz);

	}
EXIT:
	printf5( "Returning from test\n" );
	return rc;
}
