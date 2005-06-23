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
 **  FILE       : test_rt_sigaction.c
 **
 **  PURPOSE    : To test the rt_sigaction library call auditing.
 **
 **  DESCRIPTION: The test_rt_sigaction() function builds into the
 **  laus_test framework to verify that the Linux Audit System
 **  accurately logs both successful and erroneous execution of the
 **  "rt_sigaction" system call.
 **
 **  In the successful case, this function:
 **   1) Calls rt_sigaction with signum=SIGTERM
 **   2) Verifies the success result.
 **
 **  The successful case sets the signal handler for SIGTERM.  All
 **  data structures are set up for success according to the
 **  description in the man page.
 **  
 **  In the erroneous case, this function:
 **   1) Calls rt_sigaction with signum=SIGKILL
 **   2) Verifies the error result.
 **      
 **  The erroneous case attempts to set the signal handler for
 **  SIGKILL.  According to the man page for sigaction, this will
 **  result in an EINVAL error code result.
 **
 **  HISTORY    :
 **    07/03 Originated by Michael A. Halcrow <mike@halcrow.us>
 **    03/04 Added exp_errno variable by D. Kent Soper <dksoper@us.ibm.com>
 **
 **********************************************************************/
   
#include "syscalls.h"
#include <signal.h>
   
void test_rt_sighandler( int tni ) {
}
   
int test_rt_sigaction(laus_data* dataPtr) {
    
  int rc = 0;
  int exp_errno = EINVAL;
   
  int signum;
  struct sigaction act;
  struct sigaction oldact;
  size_t sigsetsize;
     
  // Set the syscall-specific data
  printf5( "Setting laus_var_data.syscallData.code to %d\n", __NR_rt_sigaction );
  dataPtr->laus_var_data.syscallData.code = __NR_rt_sigaction;
     
  /**
   * Do as much setup work as possible right here
   */
  sigsetsize = 8;
  if( dataPtr->successCase ) {
    // Set up for success
    // Might include: dataPtr->msg_euid = 0; dataPtr->msg_egid = 0;
    signum = SIGTERM;
    act.sa_handler = test_rt_sighandler;
    sigemptyset( &act.sa_mask );
    act.sa_flags = 0;
  } else {
    // Set up for error
    signum = SIGKILL;
    act.sa_handler = test_rt_sighandler;
    sigemptyset( &act.sa_mask );
    act.sa_flags = 0;
  }
   
  // Set up audit argument buffer
  //hard code the size of the &act because the one in the kernel is a different size, make platform dependent by figuring size from structure make-up
  if( ( rc = auditArg3( dataPtr,
			AUDIT_ARG_IMMEDIATE, sizeof( int ), &signum,
			AUDIT_ARG_POINTER, 
			8 + 3 *sizeof ( long ),
			&act,
			//AUDIT_ARG_POINTER, sizeof( struct sigaction ), &act,
			AUDIT_ARG_POINTER, 0, &oldact
			//AUDIT_ARG_IMMEDIATE, sizeof( size_t ), &sigsetsize 
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
  dataPtr->laus_var_data.syscallData.result = syscall( __NR_rt_sigaction, signum, &act, &oldact, sigsetsize );
   
  // Do post-system call work
  if( ( rc = postSysCall( dataPtr, errno, -1, exp_errno ) ) != 0 ) {
    printf1( "ERROR: post-syscall setup failed (%d)\n", rc );
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
