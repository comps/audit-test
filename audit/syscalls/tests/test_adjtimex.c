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
 **   1) Calls adjtimex() with buf.modes=0 (has no effect)
 **   2) Verifies the success result.
 **
 **  The successful case does not modify the timex buffer values, and
 **  so according to the man page, the call will always succeed.
 **  
 **  In the erroneous case, this function:
 **   1) Calls adjtimex() with buf.modes != 0 as the test user
 **   2) Verifies the error result.
 **      
 **  In the erroneous case, we call adjtimex as the test user.
 **  According to the man page, if buf.mode is non-zero and the user
 **  is not the superuser, then we will get an error result from the
 **  system call.
 **
 **  HISTORY    :
 **    06/03 Originated by Michael A. Halcrow <mike@halcrow.us>
 **    03/04 Added exp_errno variable by D. Kent Soper <dksoper@us.ibm.com>
 **
 **********************************************************************/

#include "syscalls.h"
#include <sys/timex.h>

#if !defined(__IX86)
struct timeval_on_disk {    // edited from /usr/include/bits/time.h
    __laus_int64 tv_sec;    /* Seconds.  */
    __laus_int64 tv_usec;   /* Microseconds.  */
};
struct timex_on_disk {      // edited from /usr/include/sys/timex.h
  __laus_int32 modes;              /* mode selector */
  __laus_int32 padding_1;	    /* PADDING */
  __laus_int64 offset;      /* time offset (usec) */
  __laus_int64 freq;        /* frequency offset (scaled ppm) */
  __laus_int64 maxerror;    /* maximum error (usec) */
  __laus_int64 esterror;    /* estimated error (usec) */
  __laus_int32 status;             /* clock command/status */
  __laus_int32 padding_2;	    /* PADDING */
  __laus_int64 constant;    /* pll time constant */
  __laus_int64 precision;   /* clock precision (usec) (read only) */
  __laus_int64 tolerance;   /* clock frequency tolerance (ppm) (read only) */

  struct timeval_on_disk time;  /* (read only) */
  __laus_int64 tick;        /* (modified) usecs between clock ticks */
  __laus_int64 ppsfreq;     /* pps frequency (scaled ppm) (ro) */
  __laus_int64 jitter;      /* pps jitter (us) (ro) */
  __laus_int32 shift;              /* interval duration (s) (shift) (ro) */
  __laus_int32 padding_3;          /* PADDING */
  __laus_int64 stabil;      /* pps stability (scaled ppm) (ro) */
  __laus_int64 jitcnt;      /* jitter limit exceeded (ro) */
  __laus_int64 calcnt;      /* calibration intervals (ro) */
  __laus_int64 errcnt;      /* calibration errors (ro) */
  __laus_int64 stbcnt;      /* stability limit exceeded (ro) */

  /* ??? */
  int  :32; int  :32; int  :32; int  :32;
  int  :32; int  :32; int  :32; int  :32;
  int  :32; int  :32; int  :32; int  :32;
};
#else
#define timeval_on_disk timeval
#define timex_on_disk   timex
#endif

int test_adjtimex(laus_data* dataPtr) {


	int rc = 0;
	int exp_errno = EPERM;

	struct timex_on_disk buf;
	struct timex syscall_buf;

	memset((char*)&buf, '\0', sizeof(buf));
	memset((char*)&syscall_buf, '\0', sizeof(syscall_buf));

	// Set the syscall-specific data
	printf5( "Setting laus_var_data.syscallData.code to %d\n", AUDIT_adjtimex );
	dataPtr->laus_var_data.syscallData.code = AUDIT_adjtimex;

	/**
	 * Do as much setup work as possible right here
	 */
	// Make sure that we don't get an EPERM in either case
	if( dataPtr->successCase ) {
		// Set up for success
		//    dataPtr->msg_euid = 0;
		//    dataPtr->msg_egid = 0;
		syscall_buf.modes = buf.modes = 0;
	} else {
		// Set up for error
		syscall_buf.modes = buf.modes = ADJ_STATUS;
		syscall_buf.status = buf.status = 42; // EPERM should hit before EINVAL, but just in case...
	}

	// Set up audit argument buffer
	if( ( rc = auditArg1( dataPtr,
					AUDIT_ARG_POINTER, sizeof( struct timex_on_disk ), &buf ) ) != 0 ) {
		printf1( "Error setting up audit argument buffer\n" );
		goto EXIT;
	}

	// Do pre-system call work
	if ( (rc = preSysCall( dataPtr )) != 0 ) {
		printf1("ERROR: pre-syscall setup failed (%d)\n", rc);
		goto EXIT_CLEANUP;
	}

	// Execute system call
	dataPtr->laus_var_data.syscallData.result = syscall( __NR_adjtimex, &syscall_buf );


	// Do post-system call work
	if ( (rc = postSysCall(  dataPtr, errno, -1, exp_errno  )) != 0 ) {
		printf1("ERROR: post-syscall setup failed (%d)\n", rc);
		goto EXIT_CLEANUP;
	}

EXIT_CLEANUP:
	/**
	 * Do cleanup work here
	 */
	if( dataPtr->successCase ) {
		// Clean up from success case setup
	}

EXIT:
	printf5( "Returning from test\n" );
	return rc;
}
