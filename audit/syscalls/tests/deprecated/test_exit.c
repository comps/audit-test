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
 **  FILE       : test_exit.c
 **
 **  PURPOSE    : To test the exit library call auditing.
 **
 **  DESCRIPTION: The test_exit() function builds into the
 **  laus_test framework to verify that the Linux Audit System
 **  accurately logs both successful and erroneous execution of the
 **  "_exit" system call.
 **
 **  In the successful case, this function:
 **   1) fork()
 **   2)  _exit(0) in the child.
 **   3)  in the parent wait for the child to exit
 **   4)  Parent handles post syscall and cleanup   
 **
 **  The successful case depends on a successful call to fork.
 **  _exit is tested from within the child.  If the parent is
 **  awakened with status the exit succeeded..
 **
 **  There is not an erroneous case for exit because there is no
 **  return code and no error codes.
 **
 **  HISTORY    :
 **    06/03 Originated by Kylene J. Smith <kylene@us.ibm.com>
 **    03/04 Added exp_errno variable by D. Kent Soper <dksoper@us.ibm.com>
 **
 **********************************************************************/
   
#include "syscalls.h"
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
   
int test_exit(laus_data* dataPtr) {
     
    
  int rc = 0;
  int exp_errno = 0;
  int status = 0;
  int pid;

  // Set the syscall-specific data
  printf5( "Setting laus_var_data.syscallData.code to %d\n", __NR_exit );
  dataPtr->laus_var_data.syscallData.code = __NR_exit;
     
   
  // Do as much setup work as possible right here
  if( !dataPtr->successCase ) {
    rc = SKIP_TEST_CASE;
    goto EXIT;
  }
   
  // Set up audit argument buffer
  if( ( rc = auditArg1( dataPtr, 
			AUDIT_ARG_IMMEDIATE, sizeof( int), &status ) ) != 0 ) {
    printf1( "Error setting up audit argument buffer\n" );
    goto EXIT;
  }
     
  // Do pre-system call work
  if ( (rc = preSysCall( dataPtr )) != 0 ) {
    printf1("ERROR: pre-syscall setup failed (%d)\n", rc);
    goto EXIT_CLEANUP;
  }
   
  // Execute system call
  pid = fork ();
  if( pid == 0 ) {
    // We are in the child
    _exit( status ); // This is what we are testing   
  } else {     // We are in the parent
    if ( pid > 0 ) { //was the fork successful?
      // Parent is going to sleep; it will be awakened by the child's exit call
      wait(&rc);
      dataPtr->msg_pid = pid;
	     
      //check that the child exited as we expected
      if ( WIFEXITED(rc) && WEXITSTATUS(rc) == status ) {
	dataPtr->laus_var_data.syscallData.result = status;
	rc = 0;
	printf5("The child exited normally with status: %d\n", 
		dataPtr->laus_var_data.syscallData.result);
      }
    } else { //fork failed
      dataPtr->laus_var_data.syscallData.result = -1;
      printf1("The fork failed; did not test exit\n");
      goto EXIT_CLEANUP;
    }
  }
   
  // Now do post-system call work.
  // There are no error conditions for exit (exp_errno = 0).
  if ( (rc = postSysCall(  dataPtr, errno, -1, exp_errno )) != 0 ) {
    printf1("ERROR: post-syscall setup failed (%d)\n", rc);
    goto EXIT_CLEANUP;
  }
   
   
 EXIT_CLEANUP:
   
  // Do cleanup work here
  if( dataPtr->successCase ) {
    // Clean up from success case setup
  }
   
 EXIT:
  printf5( "Returning from test \n" );
  return rc;
}
