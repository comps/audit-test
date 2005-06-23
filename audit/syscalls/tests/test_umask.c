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
    **  FILE       : test_umask.c
    **
    **  PURPOSE    : To test the umask library call auditing.
    **
    **  DESCRIPTION: The test_umask() function builds into the
    **  laus_test framework to verify that the Linux Audit System
    **  accurately logs both successful and erroneous execution of the
    **  "umask" system call.
    **
    **  In the successful case, this function:
    **   1) Sets the umask to 000
    **   2) Calls umask with mask=000
    **   3) Verifies the result against the successful case.
    **
    **  The umask system call is always successful, and so we must treat
    **  it as a special case in the context of this audit test suite.
    **  
    **  There is no erroneous case because umask will never fail.
    **
    **  HISTORY    :
    **    06/03 Originated by Michael A. Halcrow <mike@halcrow.us>
    **    03/04 Added exp_errno variable by D. Kent Soper <dksoper@us.ibm.com>
    **
    **********************************************************************/
   
   #include "syscalls.h"
   #include <sys/types.h>
   #include <sys/stat.h>
   
   int test_umask(laus_data* dataPtr) {
     
    
     int rc = 0;
     int exp_errno = 0;
   
     int mask;
     
     if ( ! dataPtr->successCase ) { //umask never fails
	 rc = SKIP_TEST_CASE;
	 goto EXIT;
     }

     // Set the syscall-specific data
     printf5( "Setting laus_var_data.syscallData.code to %d\n", AUDIT_umask );
     dataPtr->laus_var_data.syscallData.code = AUDIT_umask;
     
     
     //Do as much setup work as possible right here
     mask = 000;
     umask( mask );
     // Set up audit argument buffer
     if( ( rc = auditArg1( dataPtr, AUDIT_ARG_IMMEDIATE, sizeof( int ), &mask ) ) != 0 ) {
       printf1( "Error setting up audit argument buffer\n" );
       goto EXIT;
     }
   
     // Do pre-system call work
     if ( (rc = preSysCall( dataPtr )) != 0 ) {
       printf1("ERROR: pre-syscall setup failed (%d)\n", rc);
       goto EXIT_CLEANUP;
     }
   
     // Execute system call
     dataPtr->laus_var_data.syscallData.result = syscall( __NR_umask, mask );
   
     // Do post-system call work
     if ( (rc = postSysCall(  dataPtr, errno, -1, exp_errno  )) != 0 ) {
	 printf1("ERROR: post-syscall setup failed (%d)\n", rc);
	 goto EXIT_CLEANUP;
     }
   
   EXIT_CLEANUP:
     // Do cleanup work here
   
    EXIT:
     printf5( "Returning from test\n" );
     return rc;
   }
