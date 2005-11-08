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
    **  FILE       : test_signal.c
    **
    **  PURPOSE    : To test the signal library call auditing.
    **
    **  DESCRIPTION: The test_signal() function builds into the
    **  laus_test framework to verify that the Linux Audit System
    **  accurately logs both successful and erroneous execution of the
    **  "signal" system call.
    **
    **  In the successful case, this function:
    **   1) Calls signal with signum=SIGTERM
    **   2) Verifies the success result.
    **
    **  The successful case sets the signal handler for SIGTERM.
    **  
    **  In the erroneous case, this function:
    **   1) Calls signal with signum=SIGKILL
    **   2) Verifies the error result.
    **      
    **  The erroneous case attempts to set the signal handler for
    **  SIGKILL.  According to the man page, this will result in an
    **  EINVAL error code result.
    **
    **  HISTORY    :
    **    06/03 Originated by Michael A. Halcrow <mike@halcrow.us>
    **    03/04 Added exp_errno variable by D. Kent Soper <dksoper@us.ibm.com>
    **
    **********************************************************************/

#ifndef __X86_64
   
   #include "syscalls.h"
   #include <signal.h>
   
   typedef void (*sighandler_t)(int);
   
   void test_signalhandler( int tni ) {
   }
   
   int test_signal(laus_data* dataPtr) {
     
    
     int rc = 0;
     int exp_errno = EINVAL;
   
     int signum;
     sighandler_t handler;
     
     // Set the syscall-specific data
     printf5( "Setting laus_var_data.syscallData.code to %d\n", __NR_signal );
     dataPtr->laus_var_data.syscallData.code = __NR_signal;
     
     /**
      * Do as much setup work as possible right here
      */
     handler = test_signalhandler;
     if( dataPtr->successCase ) {
       // Set up for success
       // Might include: dataPtr->msg_euid = 0; dataPtr->msg_egid = 0;
       signum = SIGTERM;
     } else {
       // Set up for error
       signum = SIGKILL;
     }
   
     // Set up audit argument buffer
     if( ( rc = auditArg2( dataPtr,
			   AUDIT_ARG_IMMEDIATE, sizeof( int ), &signum,
			   AUDIT_ARG_POINTER, 0, handler ) ) != 0 ) {
	 printf1( "Error setting up audit argument buffer\n" );
	 goto EXIT;
     }
   
     // Do pre-system call work
     if ( (rc = preSysCall( dataPtr )) != 0 ) {
       printf1("ERROR: pre-syscall setup failed (%d)\n", rc);
       goto EXIT_CLEANUP;
     }
   
     // Execute system call
     dataPtr->laus_var_data.syscallData.result = syscall( __NR_signal, signum, handler );
   
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

#endif
