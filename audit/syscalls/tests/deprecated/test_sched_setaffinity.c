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
  **  FILE       : test_sched_setaffinity.c
  **
  **  PURPOSE    : To test the sched_setaffinity library call auditing.
  **
  **  DESCRIPTION: The test_sched_setaffinity() function builds into the
  **  laus_test framework to verify that the Linux Audit System
  **  accurately logs both successful and erroneous execution of the
  **  "sched_setaffinity" system call.
  **
  **  In the successful case, this function:
  **   1) Set the length and mask fields to acceptable values
  **   2) Set the pid to 0 which corresponds to the current process
  **   3) Perform the syscall
  **
  **  The successful case changes the affinity mask of the current process
  **  to 0x11 with the sched_setaffinity system call
  **  
  **  In the erroneous case, this function:
  **   1) Set the length and mask fields to acceptable values
  **   2) Set the pid to -1 an invalid value
  **   3) Preform the syscall
  **      
  **  The erroneous case causes a ESRCH error by passing an invalid
  **  process id number to the sched_setaffinity syscall.
  **
  **  HISTORY    :
  **    07/03 Originated by Kylene J. Smith <kylene@us.ibm.com>
  **    03/04 Added exp_errno variable by D. Kent Soper <dksoper@us.ibm.com>
  **
  **********************************************************************/
 #include "syscalls.h"
 #include <sched.h>
 
 int test_sched_setaffinity(laus_data* dataPtr) {
   int rc = 0;
   int exp_errno = ESRCH;

   pid_t pid;
   unsigned long len  = sizeof(unsigned long);
   unsigned long mask = 0x11;
   
   // Set the syscall-specific data
   printf5( "Setting laus_var_data.syscallData.code to %d\n", __NR_sched_setaffinity );
   dataPtr->laus_var_data.syscallData.code = __NR_sched_setaffinity;
   
   // Do as much setup work as possible right here
   if( dataPtr->successCase ) {     // Set up for success
       pid = 0;
   } else {     // Set up for error
       pid = -1;
   }
   
   // Set up audit argument buffer
   if( ( rc = auditArg3( dataPtr,
			 AUDIT_ARG_IMMEDIATE, sizeof( pid_t ), &pid,
			 AUDIT_ARG_IMMEDIATE, sizeof( unsigned long ), &len,
			 AUDIT_ARG_POINTER, sizeof( unsigned long ), &mask ) ) != 0 ) {
       printf1( "Error setting up audit argument buffer\n" );
       goto EXIT;
   }
   
   // Do pre-system call work
   preSysCall( dataPtr );
   
   // Execute system call
   dataPtr->laus_var_data.syscallData.result = syscall( __NR_sched_setaffinity, pid, len, &mask );
   
   // Do post-system call work
   postSysCall( dataPtr, errno, -1, exp_errno );
   
 EXIT_CLEANUP:
   // Do cleanup work here
   if( dataPtr->successCase ) {
       // Clean up from success case setup
   }
   
 EXIT:
   printf5( "Returning from test\n" );
   return rc;
}
