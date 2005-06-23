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
 **  FILE       : test_shmctl_einval.c
 **
 **  PURPOSE    : To test the shmctl library call auditing.
 **
 **  DESCRIPTION: The test_shmctl_einval() function builds into the laus_test
 **  framework to verify that the Linux Audit System accurately logs
 **  both successful and erroneous execution of the "lpc" system call.
 **
 **  In the successful case, this function:
 **   1) Clears the audit trail
 **   2) Allocates a new block of shared memory via shmget()
 **   3) Uses shmctl() to deallocate the newly allocated block of memory
 **   4) Tests the result of the call against the expected successful
 **      return.
 **  
 **  The successful case uses the shmid returned by shmget() in using
 **  shmctl() to deallocate the memory.  If shmctl() returns 0, we have
 **  a success, as specified in the man page.
 **
 **  In the erroneous case, this function:
 **   1) Clears the audit trail
 **   2) Uses shmctl() to attempt to deallocate a shared memory block
 **      with the invalid error identifier (-1)
 **   3) Tests the result of the call against the expected erroneous
 **      return.
 **
 **  Since -1 is documented in shmget()'s man page as the shmid value
 **  returned when an error has occured, we imply that -1 is an invalid
 **  identifier.  In the erroneous case, we expect shmctl() to return
 **  -1 on error when given -1 as the shmid, and errno should be set to
 **  EINVAL, which, according to the man page, is the value set when
 **  shmid is not a valid identifier.
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
#include <sys/shm.h>
#include <asm/page.h>
   
int test_shmctl_einval(laus_data* dataPtr) {
    
  int rc = 0;
  int exp_errno = EINVAL;
  int shmctlrc = 0;
  int shmid = 0;
  int third_arg = 0;
  key_t key;
  int mode;
  static int cmd = IPC_RMID;
  struct shmid_ds buf;
   
  // Set the syscall-specific data
  printf5( "Setting laus_var_data.syscallData.code to %d\n", AUDIT_shmctl );
  dataPtr->laus_var_data.syscallData.code = AUDIT_shmctl;   

  /**
   * Do as much setup work as possible right here
   */
  // su to test user
  printf5( "seteuid to %i\n", dataPtr->msg_euid );
  if( ( rc = seteuid( dataPtr->msg_euid ) ) != 0 ) {
    printf1( "ERROR: Unable to seteuid to %i: errno=%i\n", 
	     dataPtr->msg_euid, errno );
    goto EXIT;
  }

  memset(&buf, 0, sizeof(buf));

  if( dataPtr->successCase ) {
    mode = S_IRWXU | S_IRWXG | S_IRWXO;
    // Allocate shared memory space so that we can test deallocation via shmct
    if( ( shmid = shmget( IPC_PRIVATE, PAGE_SIZE, mode ) ) == -1 ) {
      printf1( "ERROR: Unable to allocate new shared memory segment: errno=%i\n", errno );
      goto EXIT;
    }
  } else {
    shmid = -1;
  }
  // su back to root
  if ( ( rc = seteuid( 0 ) ) != 0 ) {
    printf1( "ERROR: Unable to seteuid to root: errno=%i\n", errno );
    goto EXIT;
  }


  // Set up audit argument buffer

#ifndef __X86_64
  if( ( rc = auditArg3( dataPtr,
			AUDIT_ARG_IMMEDIATE, sizeof( int ), &shmid,
			AUDIT_ARG_IMMEDIATE, sizeof( int ), &cmd,
#ifndef __PPC64
			AUDIT_ARG_POINTER, 48, &buf 
#else
			AUDIT_ARG_POINTER, 88, &buf 
#endif
			) ) != 0 ) {
#else
  if ( ( rc = auditArg4( dataPtr,
			 AUDIT_ARG_IMMEDIATE, sizeof(int), &shmid,
			 AUDIT_ARG_IMMEDIATE, sizeof(int), &cmd,
			 AUDIT_ARG_IMMEDIATE, sizeof(int), &third_arg,
			 AUDIT_ARG_POINTER, 0, &buf ) ) != 0 ) {
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
  dataPtr->laus_var_data.syscallData.result = shmctlrc = syscall( __NR_ipc, SHMCTL,
								  shmid, IPC_RMID, 0, &buf );
#else
  dataPtr->laus_var_data.syscallData.result = shmctlrc = syscall( __NR_shmctl,
								  shmid, IPC_RMID, 0, &buf );
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
   
