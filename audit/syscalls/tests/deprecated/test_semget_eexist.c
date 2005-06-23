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
 **  FILE       : test_semget_eexist.c
 **
 **  PURPOSE    : To test the semget library call auditing.
 **
 **  DESCRIPTION: The test_semget_eexist() function builds into the laus_test
 **  framework to verify that the Linux Audit System accurately logs
 **  both successful and erroneous execution of the "lpc" system call.
 **
 **  In the successful case, this function:
 **   1) Sets the semaphore mode flags
 **   2) Clears the audit trail
 **   3) Sets key to IPC_PRIVATE
 **   4) Executes the "semget" library call
 **   5) Tests the result of the call against the expected successful
 **      return
 **   6) Deallocates the newly allocated semaphore.
 **  
 **  The successful case executes the expected conditions
 **  described by the "semget" library call man page.  That is,
 **  the semget() function is called using IPC_PRIVATE for the key
 **  value.  "mode" is set to ( S_IRWXU | S_IRWXG | S_IRWXO ).  "nsems"
 **  is set to 1.  The function returns an integer value specifying the
 **  semaphore identifier for the newly allocated semaphore; the
 **  identifier should be a valid value (not NULL or -1).
 **
 **  In the erroneous case, this function:
 **   1) Sets the semaphore mode flags
 **   2) Clears the audit trail
 **   3) Attempts to allocate a semaphore with key=-1 by
 **      calling semget().  In the event that this semaphore is
 **      already allocated, the doNotDeallocate flag is set to "1".
 **   4) Executes the "semget" library call with key=-1
 **   5) Tests the result of the call against the expected erroneous
 **      return
 **   6) Exits, in the event that we have set the doNotDeallocate flag.
 **      If the doNotDeallocate flag is not set, then we deallocate the
 **      allocated semaphore associated with key=-1.
 **
 **  The erroneous case forces an error condition due to the fact that
 **  we attempt to allocate the semaphore associated with key=-1
 **  twice.  The function returns an integer value "-1" to indicate the
 **  error condition, and "errno" is set to "EEXIST".
 **
 **  HISTORY    :
 **    06/03 Originated by Michael Halcrow <mike@halcrow.us>
 **    06/03 Furthered by Dustin Kirkland (k1rkland@us.ibm.com)
 **    03/04 Added exp_errno variable by D. Kent Soper <dksoper@us.ibm.com>
 **
 **********************************************************************/
   
#include "syscalls.h"
#include <sys/types.h>
#include <sys/ipc.h>
#include "../../include/asm/ipc.h"
#include <sys/sem.h>
   
int test_semget_eexist(laus_data* dataPtr) {
    
    
  int rc = 0;
  int exp_errno = EEXIST;
  int key = 0;
  int firstSemid = 0;
  int secondSemid = 0;
  int doNotDeallocate = 0;
  int mode;
  int nsems = 1;
   
   
   
  // Set the syscall-specific data
  dataPtr->laus_var_data.syscallData.code = AUDIT_semget;
   
  // Set the mode flags
  mode = S_IRWXU | S_IRWXG | S_IRWXO;
   
  // Set the number of semaphores for the identifier
  nsems = 1;
   
  // su to test user
  printf5( "seteuid to %i\n", dataPtr->msg_euid );
  if( ( rc = seteuid( dataPtr->msg_euid ) ) != 0 ) {
    printf1( "ERROR: Unable to seteuid to %i: errno=%i\n", 
	     dataPtr->msg_euid, errno );
    goto EXIT;
  }
   
  // Set the key value.
  // If successCase == 0, then we will be double-allocating the memory
  // to force an error condition. 
  if( dataPtr->successCase ) {
    key = IPC_PRIVATE;
  } else {    
    mode |= ( IPC_CREAT | IPC_EXCL );
    key = -1;
    if( ( firstSemid = semget( key, nsems, mode ) ) == -1 ) {
      // Okay; so for some reason, -1 is already allocated. No matter;
      // we just need to check for the audit record.  But don't
      // deallocate it at the end of the test!
      doNotDeallocate = 1;
    }
  }
   
  // su back to root
  printf5( "seteuid to root\n" );
  if ( ( rc = seteuid( 0 ) ) != 0 ) {
    printf1( "ERROR: Unable to seteuid to root: errno=%i\n", errno );
    goto EXIT_FREE_SEM;
  }

  // Set up audit argument buffer
  if( ( rc = auditArg3( dataPtr,
		      AUDIT_ARG_IMMEDIATE, sizeof( int ), &key,
		      AUDIT_ARG_IMMEDIATE, sizeof( int ), &nsems,
		      AUDIT_ARG_IMMEDIATE, sizeof( int ), &mode ) ) != 0 ) {
    printf1( "Error setting up audit argument buffer\n" );
    goto EXIT;
  }

  // Do pre-system call work   
  preSysCall( dataPtr );
   
  // Execute the semget system call
#ifndef __X86_64
  dataPtr->laus_var_data.syscallData.result = secondSemid = syscall( __NR_ipc, SEMGET,
								     key, nsems, mode );
#else
  dataPtr->laus_var_data.syscallData.result = secondSemid = syscall( __NR_semget,
								     key, nsems, mode );
#endif

  // Do post-system call work
  postSysCall( dataPtr, errno, -1, exp_errno );
  
 EXIT_FREE_SEM:
  if( !doNotDeallocate ) {
    if( firstSemid && ( firstSemid != -1 ) ) {
      if( ( semctl( firstSemid, 0, IPC_RMID ) ) == -1 ) {
   	printf1( "ERROR: Cannot deallocate message memory with semid=%d: errno=%i\n", 
		 firstSemid, errno );
   	goto EXIT;
      }
    }    
    if( secondSemid && ( secondSemid != -1 ) ) {
      if( ( semctl( secondSemid, 0, IPC_RMID ) ) == -1 ) {
   	printf1( "ERROR: Cannot deallocate message memory with semid=%d: errno=%i\n", 
		 secondSemid, errno );
   	goto EXIT;
      }
    }
  }
  
 EXIT:
  return rc;
}
