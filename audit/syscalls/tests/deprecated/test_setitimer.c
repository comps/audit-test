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
    **  FILE       : test_setitimer.c
    **
    **  PURPOSE    : To test the setitimer library call auditing.
    **
    **  DESCRIPTION: The test_setitimer() function builds into the
    **  laus_test framework to verify that the Linux Audit System
    **  accurately logs both successful and erroneous execution of the
    **  "setitimer" system call.
    **
    **  In the successful case, this function:
    **   1) Retrieves the current ITIMER_VIRTUAL structure values
    **   2) Makes the setitimer call with the retrieved values
    **   3) Verifies the success result of the system call.
    **
    **  The successful case retrieves a known good set of values for the
    **  itimerval data structure, and then it uses those values in the
    **  call to set the timer.  Since all pointers are valid and since
    **  the specified timer (ETIMER_VIRTUAL) exists, there is no possible
    **  error condition in the successful case.
    **  
    **  In the erroneous case, this function:
    **   1) Attempts to set a non-existent timer.
    **   2) Verifies the error result of the system call.
    **      
    **  The erroneous case causes the setitimer system call to result in
    **  an EINVAL error condition by passing in a ``which'' that is not
    **  one of ITIMER_REAL, ITIMER_VIRT, or ITIMER_PROF.
    **
    **  HISTORY    :
    **    06/03 Originated by Michael A. Halcrow <mike@halcrow.us>
    **    03/04 Added exp_errno variable by D. Kent Soper <dksoper@us.ibm.com>
    **
    **********************************************************************/
   
   #include "includes.h"
   #include <sys/time.h>
   
   int test_setitimer(laus_data* dataPtr) {
     
    
     int rc = 0;
     int exp_errno = EINVAL;
   
     int which;
     struct itimerval value;
     struct itimerval ovalue; // For storing the old value of the timer
     
     // Set the syscall-specific data
     printf5( "Setting laus_var_data.syscallData.code to %d\n", __NR_setitimer );
     dataPtr->laus_var_data.syscallData.code = __NR_setitimer;
     
     /**
      * Do as much setup work as possible right here
      */
     if( dataPtr->successCase ) {
       // Set up for success
       // Might include: dataPtr->msg_euid = 0; dataPtr->msg_egid = 0;
       which = ITIMER_VIRTUAL; // As good as any
       if( getitimer( which, &value ) == -1 ) {
         printf1( "Error getting timer value\n" );
         goto EXIT;
       }
     } else {
       // Set up for error
       which = 42;
       while( which == ITIMER_VIRTUAL || which == ITIMER_REAL || which == ITIMER_PROF )
         which++;
     }
   
     // Set up audit argument buffer
     if( ( rc = auditArg3( dataPtr, 
			   AUDIT_ARG_IMMEDIATE, sizeof( int ), &which,
			   AUDIT_ARG_POINTER, 0, &value,
			   AUDIT_ARG_POINTER, 0, &ovalue ) ) != 0 ) {
       printf1( "Error setting up audit argument buffer\n" );
       goto EXIT;
     }
   
     // Do pre-system call work
     if ( (rc = preSysCall( dataPtr )) != 0 ) {
       printf1("ERROR: pre-syscall setup failed (%d)\n", rc);
       goto EXIT_CLEANUP;
     }
   
     // Execute system call
     dataPtr->laus_var_data.syscallData.result = syscall( __NR_setitimer, which, &value, &ovalue );
   
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
