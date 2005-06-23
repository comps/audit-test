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
 **  FILE       : test_semctl_einval.c
 **
 **  PURPOSE    : To test the semctl library call auditing.
 **
 **  DESCRIPTION: The test_semctl_einval() function builds into the laus_test
 **  framework to verify that the Linux Audit System accurately logs
 **  both successful and erroneous execution of the "lpc" system call.
 **
 **  In the successful case, this function:
 **   1) Allocates a new semaphore via semget()
 **   2) Uses semctl() to deallocate the newly allocated semaphore
 **   3) Tests the result of the call against the expected successful
 **      return.
 **  
 **  The successful case uses the semid returned by semget() in using
 **  semctl() to deallocate the memory.  If semctl() returns 0, we have
 **  a success, as specified in the man page.
 **
 **  In the erroneous case, this function:
 **   1) Uses semctl() to attempt to deallocate a semaphore
 **      with the error identifier (-1), which is invalid
 **   2) Tests the result of the call against the expected erroneous
 **      return.
 **
 **  Since -1 is documented in semget()'s man page as the semid value
 **  returned when an error has occured, we infer that -1 is an invalid
 **  identifier.  In the erroneous case, we expect semctl() to return
 **  -1 on error when given -1 as the semid, and errno should be set to
 **  EINVAL, which, according to the man page, is the value set when
 **  semid is not a valid identifier.
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
   
int test_semctl_einval(laus_data* dataPtr) {
    
    
  int rc = 0;
  int exp_errno = EINVAL;
  int semctlrc = 0;
  int semid = 0;
  key_t key;
  int mode;
  int semnum = 0; // this is ignored by semctl when we remove
  static int cmd = IPC_RMID;
  struct semid_ds buf;
   
  syscall_data* syscallData = (syscall_data*)dataPtr;
   
  // Set the syscall-specific data
  dataPtr->laus_var_data.syscallData.code = AUDIT_semctl;
   
  // su to test user
  printf5( "seteuid to %i\n", dataPtr->msg_euid );
  if( ( rc = seteuid( dataPtr->msg_euid ) ) != 0 ) {
    printf1( "ERROR: Unable to seteuid to %i: errno=%i\n", 
	     dataPtr->msg_euid, errno );
    goto EXIT;
  }
   
  // Allocate shared memory space so that we can test deallocation via
  // semctl
  if( dataPtr->successCase ) {
    mode = S_IRWXU | S_IRWXG | S_IRWXO;
    if( ( semid = semget( IPC_PRIVATE, 1, mode ) ) == -1 ) {
      printf1( "ERROR: Unable to allocate new semaphore: errno=%i\n", errno );
      goto EXIT;
    }
  } else {
    semid = -1;
  }
   
  // su back to root
  printf5( "seteuid to root\n" );
  if ( ( rc = seteuid( 0 ) ) != 0 ) {
    printf1( "ERROR: Unable to seteuid to root: errno=%i\n", errno );
    goto EXIT;
  }

  printf("[%d]\n", cmd);
   
  // Set up audit argument buffer
#ifndef __X86_64
  if( ( rc = auditArg4( dataPtr,
			AUDIT_ARG_IMMEDIATE, sizeof( int ), &semid,
			AUDIT_ARG_IMMEDIATE, sizeof( int ), &semnum,
			AUDIT_ARG_IMMEDIATE, sizeof( int ), &cmd,
			AUDIT_ARG_POINTER, 0, &buf
			) ) != 0 ) {
#else
  if( ( rc = auditArg3( dataPtr,
			AUDIT_ARG_IMMEDIATE, sizeof( int ), &semid,
			AUDIT_ARG_IMMEDIATE, sizeof( int ), &semnum,
			AUDIT_ARG_NULL, 0, NULL
			) ) != 0 ) {
#endif
    printf1( "Error setting up audit argument buffer\n" );
    goto EXIT;
  }
   
  // Do pre-system call work
  if ( (rc = preSysCall( dataPtr )) != 0 ) {
    printf1("ERROR: pre-syscall setup failed (%d)\n", rc);
    goto EXIT_CLEANUP;
  }
   
  // Execute system call
#ifndef __X86_64
  dataPtr->laus_var_data.syscallData.result = semctlrc = syscall( __NR_ipc, SEMCTL, semid, 0, cmd, &buf );
#else
  dataPtr->laus_var_data.syscallData.result = semctlrc = syscall( __NR_semctl, semid, 0, cmd, NULL );
#endif
  // Do post-system call work
  if ( (rc = postSysCall(  dataPtr, errno, -1, exp_errno  )) != 0 ) {
    printf1("ERROR: post-syscall setup failed (%d)\n", rc);
    goto EXIT_CLEANUP;
  }
   
  
 EXIT_CLEANUP:
   
 EXIT:
  return rc;
}
   
