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
 **  FILE       : test_shmat_einval.c
 **
 **  PURPOSE    : To test the shmat library call auditing.
 **
 **  DESCRIPTION: The test_shmat() function builds into the laus_test
 **  framework to verify that the Linux Audit System accurately logs
 **  both successful and erroneous execution of the "lpc" system call.
 **
 **  In the successful case, this function:
 **   1) Allocates a new block of shared memory
 **   2) Clears the audit trail
 **   3) Attaches to that block of shared memory
 **   4) Tests the result of the shmat() call against the expected
 **      successful return
 **   5) Detaches from the block of shared memory
 **   6) Deallocates the block of shared memory.
 **  
 **  The successful case attaches to the shared memory block specified
 **  by the shmid value returned by shmget().
 **
 **  In the erroneous case, this function:
 **   1) Clears the audit trail
 **   2) Attempts to attach using an invalid shared memory identifier
 **   3) Tests the results of the shmat() call against the expected
 **      erroneous return
 **
 **  In the erroneous case, we attempt to attach to shared memory
 **  identifier -1, which is the invalid identifier.  According to the
 **  man page for shmat(), we should expect a -1 result, and errno
 **  should be set to EINVAL.
 **
 **  HISTORY    :
 **    06/03 Originated by Michael A. Halcrow <mike@halcrow.us>
 **    06/03 Furthered by Dustin Kirkland  (k1rkland@us.ibm.com)
 **    03/04 Added exp_errno variable by D. Kent Soper <dksoper@us.ibm.com>
 **
 **********************************************************************/
   
#include "syscalls.h"
#include <sys/types.h>
#include <sys/shm.h>
#include <asm/page.h>
#include "../../include/asm/ipc.h"
   
int test_shmat_einval(laus_data* dataPtr) {
    
  int rc = 0;
  int exp_errno = EINVAL;
  int shmid = 0;
  static void *voidPtr = NULL;
  int shmflg = 0;
  void* shmptr = NULL;
  unsigned long raddr = 0;
  int mode;
   
   
  // Set the syscall-specific data
  printf5( "Setting laus_var_data.syscallData.code to %d\n", AUDIT_shmat );
  dataPtr->laus_var_data.syscallData.code = AUDIT_shmat;

  /**
   * Do as much setup work as possible right here
   */
  // Create the shared memory segment
  if( dataPtr->successCase ) {
    mode = S_IRWXU | S_IRWXG | S_IRWXO;
    if( ( shmid = shmget( IPC_PRIVATE, PAGE_SIZE, mode ) ) == -1 ) {
      printf1( "ERROR: Unable to create shared memory segment\n" );
      goto EXIT;
    }
    printf4( "Generated shmid = [%d]\n", shmid );
  } else {
    shmid = -1;
  }
   
   
  // Do pre-system call work
  if ( (rc = preSysCall( dataPtr )) != 0 ) {
    printf1("ERROR: pre-syscall setup failed (%d)\n", rc);
    goto EXIT_CLEANUP;
  }
   
  // Execute system call
#ifndef __X86_64
  //shmptr = shmat( shmid, NULL, 0 );
  shmptr = syscall( __NR_ipc, SHMAT, shmid, 0, &raddr, NULL );
#else
  shmptr = syscall ( AUDIT_shmat, shmid, NULL, 0, &raddr );
#endif
  if ( ((long)shmptr) != -1 ) { 
#ifndef __X86_64
      dataPtr->laus_var_data.syscallData.result = 0;
#else
      dataPtr->laus_var_data.syscallData.result = shmptr;
#endif
  } else {
      dataPtr->laus_var_data.syscallData.result = -1;
      dataPtr->laus_var_data.syscallData.resultErrno = errno;
  }

  //Strange location because kernel filling value in raddr that we need
  // Set up audit argument buffer
  if( ( rc = auditArg4( dataPtr,
                        AUDIT_ARG_IMMEDIATE, sizeof( int ), &shmid,
                        AUDIT_ARG_NULL, 0, voidPtr,
                        AUDIT_ARG_IMMEDIATE, sizeof( int ), &shmflg,
                        AUDIT_ARG_POINTER, sizeof(unsigned long), &raddr
                        ) ) != 0 ) {
    printf1( "Error setting up audit argument buffer\n" );
    goto EXIT;
  }

  // Do post-system call work
  if ( (rc = postSysCall(  dataPtr, errno, -1, exp_errno  )) != 0 ) {
    printf1("ERROR: post-syscall setup failed (%d)\n", rc);
    goto EXIT_CLEANUP;
  }
   
   
 EXIT_CLEANUP:
  // shared memory cleanup
  if( dataPtr->successCase ) {
    // detach memory if successfully attached
    if( (long)shmptr != -1 ) {
#ifndef __X86_64
    if ( shmdt( raddr )	== -1 ) {
#else  
    if( shmdt( shmptr ) == -1 ) {
#endif
	printf1( "ERROR: Unable to detach memory with shmid %d: errno=%i\n",
		 shmid, errno );
	goto EXIT;
      }
    }
    if( shmctl( shmid, IPC_RMID, 0 ) == -1 ) {
      printf1("ERROR: Unable to free shared memory with shmid %d: errno=%i\n", 
	      shmid, errno);
      goto EXIT;
    }
  }
       
 EXIT:
  return rc;
}
   
