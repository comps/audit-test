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
    **  FILE       : test_sched_setparam.c
    **
    **  PURPOSE    : To test the setparam library call auditing.
    **
    **  DESCRIPTION: The test_sched_setparam() function builds into the
    **  laus_test framework to verify that the Linux Audit System
    **  accurately logs both successful and erroneous execution of the
    **  "setparam" system call.
    **
    **  In the successful case, this function:
    **   1) Calls sched_param with pid=0 and p.sched_priority=0
    **   2) Verifies the success result.
    **
    **  The successful case uses a sched_priority value (0) that is valid
    **  for all scheduling policies supported by the kernel.  With pid=0,
    **  sched_setparam will perform the operation on the currently
    **  running process.
    **  
    **  In the erroneous case, this function:
    **   1) Calls sched_param with pid=0 and p.sched_priority=-99999
    **   2) Verifies the error result.
    **      
    **  The erroneous case passes in a sched_priority value that is
    **  guaranteed to be outside the acceptable range of values for any
    **  of the scheduling policies supported by the kernel.  According to
    **  the man page, when sched_setpriority is passed a struct that
    **  contains an invalid value, it returns an error and sets errno to
    **  EINVAL.
    **
    **  HISTORY    :
    **    06/03 Originated by Michael A. Halcrow <mike@halcrow.us>
    **    03/04 Added exp_errno variable by D. Kent Soper <dksoper@us.ibm.com>
    **
    **********************************************************************/
   #include "syscalls.h"
   #include <sched.h>
   
   int test_sched_setparam(laus_data* dataPtr) {
     
     int rc = 0;
     int exp_errno = EINVAL;
   
     pid_t pid;
     struct sched_param p;
     
     // Set the syscall-specific data
     printf5( "Setting laus_var_data.syscallData.code to %d\n", __NR_sched_setparam );
     dataPtr->laus_var_data.syscallData.code = __NR_sched_setparam;
     
     /**
      * Do as much setup work as possible right here
      */
     pid = 0;
     if( dataPtr->successCase ) {
       // Set up for success
       p.sched_priority = 0;
     } else {
       // Set up for error
       p.sched_priority = -99999; // Out of range; TODO: make absolutely sure of this!
     }
   
     // Set up audit argument buffer
     if( ( rc = auditArg2( dataPtr,
 			AUDIT_ARG_IMMEDIATE, sizeof( pid ), &pid,
 			AUDIT_ARG_POINTER, sizeof( struct sched_param ), &p ) ) != 0 ) {
       printf1( "Error setting up audit argument buffer\n" );
       goto EXIT;
     }
   
     // Do pre-system call work
     if ( (rc = preSysCall( dataPtr )) != 0 ) {
       printf1("ERROR: pre-syscall setup failed (%d)\n", rc);
       goto EXIT_CLEANUP;
     }
   
     // Execute system call
     dataPtr->laus_var_data.syscallData.result = syscall( __NR_sched_setparam, pid, &p );
   
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
