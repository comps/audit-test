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
 **  FILE       : test_shmget_eexist.c
 **
 **  PURPOSE    : To test the shmget library call auditing.
 **
 **  DESCRIPTION: The test_shmget_eexist() function builds into the laus_test
 **  framework to verify that the Linux Audit System accurately logs
 **  both successful and erroneous execution of the "lpc" system call.
 **
 **  In the successful case, this function:
 **   1) Sets the shared memory mode flags
 **   2) Clears the audit trail
 **   3) Sets key to IPC_PRIVATE
 **   4) Executes the "shmget" library call
 **   5) Tests the result of the call against the expected successful
 **      return
 **   6) Deallocates the newly allocated block of shared memory.
 **  
 **  The successful case executes the expected conditions
 **  described by the "shmget" library call man page.  That is,
 **  the shmget() function is called using IPC_PRIVATE for the key
 **  value.  "size" is set to PAGE_SIZE, and "mode" is set to ( S_IRWXU
 **  | S_IRWXG | S_IRWXO ).  The function returns an integer value
 **  specifying the shared memory ID for the newly allocated block of
 **  memory, which should be a valid value (not NULL or -1).
 **
 **  In the erroneous case, this function:
 **   1) Sets the shared memory mode flags
 **   2) Clears the audit trail
 **   3) Attempts to allocate a shared memory block with key=-1 by
 **      calling shmget().  In the event that this block of memory is
 **      already allocated, the doNotDeallocate flag is set to "1".
 **   4) Executes the "shmget" library call
 **   5) Tests the result of the call against the expected erroneous
 **      return
 **   6) Exits, in the event that we have set the doNotDeallocate flag.
 **      If the doNotDeallocate flag is not set, then we deallocate the
 **      allocated shared memory block associated with key=-1.
 **
 **  The erroneous case forces an error condition due to the fact that
 **  we attempt to allocate the block of memory associated with key=-1
 **  twice.  The function returns an integer value "-1" to indicate the
 **  error condition, and "errno" is set to "EEXIST".
 **
 **  HISTORY    :
 **    06/03 Originated by Michael A. Halcrow <mike@halcrow.us>
 **    06/03 Furthered by Dustin Kirkland (k1rkland@us.ibm.com)
 **    03/04 Added exp_errno variable by D. Kent Soper <dksoper@us.ibm.com>
 **
 **********************************************************************/
   
#include "syscalls.h"
#include <sys/ipc.h>
#include "../../include/asm/ipc.h"
#include <asm/page.h>
#include <sys/shm.h>
   
int test_shmget_eexist(laus_data* dataPtr) {
    
    
  int rc = 0;
  int exp_errno = EEXIST;
  int key = 0;
  int firstShmid = 0;
  int secondShmid = 0;
  int doNotDeallocate = 0;
  int mode;
  static int pageSize = PAGE_SIZE;
   
   
   
  // Set the syscall-specific data
  dataPtr->laus_var_data.syscallData.code = AUDIT_shmget;

  // Set the mode flags
  mode = S_IRWXU | S_IRWXG | S_IRWXO;
   
  // su to test user
  printf5("seteuid to %i\n", dataPtr->msg_euid );
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
    if( ( firstShmid = shmget( key, PAGE_SIZE, mode ) ) == -1 ) {
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
    goto EXIT_CLEANUP;
  }
   
  // Set up audit argument buffer
  if( ( rc = auditArg3( dataPtr,
		      AUDIT_ARG_IMMEDIATE, sizeof( int ), &key,
		      AUDIT_ARG_IMMEDIATE, sizeof( int ), &pageSize,
		      AUDIT_ARG_IMMEDIATE, sizeof( int ), &mode ) ) != 0 ) {
    printf1( "Error setting up audit argument buffer\n" );
    goto EXIT;
  }
   
  // Do pre-system call work
  if ( (rc = preSysCall( dataPtr )) != 0 ) {
    printf1("ERROR: pre-syscall setup failed (%d)\n", rc);
    goto EXIT_CLEANUP;
  }
   
  // Execute system call
  //  dataPtr->laus_var_data.syscallData.result = secondShmid = shmget( key, PAGE_SIZE, mode );
#ifndef __X86_64
  dataPtr->laus_var_data.syscallData.result = secondShmid = syscall( __NR_ipc, SHMGET,
								     key, PAGE_SIZE, mode );
#else
  dataPtr->laus_var_data.syscallData.result = secondShmid = syscall( __NR_shmget,
								     key, PAGE_SIZE, mode );
#endif   
  // Do post-system call work
  if ( (rc = postSysCall(  dataPtr, errno, -1, exp_errno  )) != 0 ) {
    printf1("ERROR: post-syscall setup failed (%d)\n", rc);
    goto EXIT_CLEANUP;
  }
   
   
 EXIT_CLEANUP:
  if( !doNotDeallocate ) {
    if( firstShmid && ( firstShmid != -1 ) ) {
      if( ( shmctl( firstShmid, IPC_RMID, 0 ) ) == -1 ) {
   	printf1( "ERROR: Cannot deallocate shared memory with shmid=%d: errno=%i\n", 
		 firstShmid, errno );
   	goto EXIT;
      }
    }    
    if( secondShmid && ( secondShmid != -1 ) ) {
      if( ( shmctl( secondShmid, IPC_RMID, 0 ) ) == -1 ) {
   	printf1( "ERROR: Cannot deallocate shared memory with shmid=%d: errno=%i\n", 
		 secondShmid, errno );
   	goto EXIT;
      }
    }
  }
       
 EXIT:
  return rc;
}
   
