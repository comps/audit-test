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
 **  FILE       : test_fork.c
 **
 **  PURPOSE    : To test the fork library call auditing.
 **
 **  DESCRIPTION: The test_fork() function builds into the
 **  laus_test framework to verify that the Linux Audit System
 **  accurately logs both successful and erroneous execution of the
 **  "fork" system call.
 **
 **  In the successful case, this function:
 **   1) Calls fork().  The child process immediately terminates.
 **   2) Compares the result against the expected result for the
 **      success case.
 **
 **  The successful case depends only on there being sufficient
 **  memory resources in the system for the call in the first place.
 **  
 **  This test does not test the erroneous case, since the only two
 **  errors defined in the man page for fork() both involve running
 **  out of  system resources.  Since running out of resources would
 **  invalidate the test environment, we cannot produce the error
 **  condition.
 **
 **  HISTORY    :
 **    06/03 Originated by Michael A. Halcrow <mike@halcrow.us>
 **    03/04 Added exp_errno variable by D. Kent Soper <dksoper@us.ibm.com>
 **    04/21 Added support for 2.6 Laus implementation by Kimberly D. Simon <kdsimon@us.ibm.com>
 **
 **********************************************************************/
   
#include "includes.h"
#include "syscalls.h"
#include <sched.h>
   
int test_fork(laus_data* dataPtr) {
     
    
  int rc = 0;
  int exp_errno = 0; 
  int flags = CLONE_CHILD_CLEARTID|CLONE_CHILD_SETTID|0x11;
     
  // Set the syscall-specific data
  printf5( "Setting laus_var_data.syscallData.code to %d\n", AUDIT_clone );
  dataPtr->laus_var_data.syscallData.code = AUDIT_clone;
     
  /**
   * Do as much setup work as possible right here
   */
  if( dataPtr->successCase ) {
    // Set up for success
    // Might include: dataPtr->msg_euid = 0; dataPtr->msg_egid = 0;;
  } else {
    rc = SKIP_TEST_CASE;
    goto EXIT;
    // Set up for error
  }
   
  // Set up audit argument buffer
  if( ( rc = auditArg1( dataPtr, 
			AUDIT_ARG_IMMEDIATE, sizeof(int), &flags 
			 ) ) != 0 ) {
   
    printf1( "Error setting up audit argument buffer\n" );
    goto EXIT;
  }
   
  // Do pre-system call work
  if ( (rc = preSysCall( dataPtr )) != 0 ) {
    printf1("ERROR: pre-syscall setup failed (%d)\n", rc);
    goto EXIT_CLEANUP;
  }
   
  // Execute system call
  if( ( dataPtr->laus_var_data.syscallData.result = fork() ) == 0 ) {
    // We are in the child
    _exit( 0 ); // This must happen before we leave this scope
  } else {
    // We are in the parent
  }
   
  // Do post-system call work
  /**
   * The error conditions for fork() require that there be
   * insufficient memory resources for the requested operation.
   * Generating this error would invalidate the test environment, and
   * so the error condition is not tested for fork().
   */
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
