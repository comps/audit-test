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
  **  FILE       : test_ioperm.c
  **
  **  PURPOSE    : To test the ioperm library call auditing.
  **
  **  DESCRIPTION: The test_ioperm() function builds into the
  **  laus_test framework to verify that the Linux Audit System
  **  accurately logs both successful and erroneous execution of the
  **  "ioperm" system call.
  **
  **  In the successful case, this function:
  **   2) Calls ioperm with from=0x282, num=1, turn_on=1 as the
  **      superuser
  **   3) Verifies the success result.
  **
  **  The successful case passes a valid value for from, num, and
  **  turn_on (respectively) to the ioperm() function.  It also
  **  executes as the superuser.  We can thus expect a success result.
  **  
  **  In the erroneous case, this function:
  **   1) Sets the euid to the test user
  **   2) Calls ioperm with from=0x282, num=1, turn_on=1
  **   3) Verifies the error result.
  **      
  **  The erroneous case sets the euid to the test user before making
  **  the ioperm call.  According to the ioperm man page, the use of
  **  ioperm requires root privileges.
  **
  **  HISTORY    :
  **    06/03 Originated by Michael A. Halcrow <mike@halcrow.us>
  **    03/04 Added exp_errno variable by D. Kent Soper <dksoper@us.ibm.com>
  **
  **********************************************************************/

#ifdef __IX86
   
 #include "includes.h"
 #include "syscalls.h"
   
 int test_ioperm(laus_data* dataPtr) {
     
   int rc = 0;
   int exp_errno = EPERM;

   unsigned long from;
   unsigned long num;
   int turn_on;
     
   // Set the syscall-specific data
   printf5( "Setting laus_var_data.syscallData.code to %d\n", AUDIT_ioperm );
   dataPtr->laus_var_data.syscallData.code = AUDIT_ioperm;
     
   /**
    * Do as much setup work as possible right here
    */
   from = 0x282;
   num = 1;
   turn_on = 1;

   if( dataPtr->successCase ) {
     // Set up for success
     dataPtr->msg_euid = 0;
     dataPtr->msg_egid = 0;
     dataPtr->msg_fsuid = 0;
     dataPtr->msg_fsgid = 0;
   } else {
     // Set up for error
   }
   
   // Set up audit argument buffer
   if( ( rc = auditArg3( dataPtr,
 		      AUDIT_ARG_IMMEDIATE, sizeof( unsigned long ), &from,
 		      AUDIT_ARG_IMMEDIATE, sizeof( unsigned long ), &num,
 		      AUDIT_ARG_IMMEDIATE, sizeof( int ), &turn_on ) ) != 0 ) {
     printf1( "Error setting up audit argument buffer\n" );
     goto EXIT;
   }
   
   // Do pre-system call work
   if ( (rc = preSysCall( dataPtr )) != 0 ) {
     printf1("ERROR: pre-syscall setup failed (%d)\n", rc);
     goto EXIT_CLEANUP;
   }
   
   // Execute system call
   dataPtr->laus_var_data.syscallData.result = syscall( __NR_ioperm, from, num, turn_on );
   
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
