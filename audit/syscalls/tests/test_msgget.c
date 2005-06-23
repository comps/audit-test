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
 **  FILE       : test_msgget.c
 **
 **  PURPOSE    : To test the msgget library call auditing.
 **
 **  DESCRIPTION: The test_msgget() function builds into the laus_test
 **  framework to verify that the Linux Audit System accurately logs
 **  both successful and erroneous execution of the "lpc" system call.
 **
 **  In the successful case, this function:
 **   1) Sets the message queue mode flags
 **   2) Sets key to IPC_PRIVATE
 **   3) Executes the "msgget" library call
 **   4) Tests the result of the call against the expected successful
 **      return
 **   5) Deallocates the newly allocated message queue.
 **  
 **  The successful case executes the expected conditions
 **  described by the "msgget" library call man page.  That is,
 **  the msgget() function is called using IPC_PRIVATE for the key
 **  value.  "mode" is set to ( S_IRWXU | S_IRWXG | S_IRWXO ).  The
 **  function returns an integer value specifying the message queue
 **  identifier for the newly allocated message queue; the identifier
 **  should be a valid value (not NULL or -1).
 **
 **  In the erroneous case, this function:
 **
 **  Message queue operations are attempted as a non-root user, thus
 **  causing an EACCES errno.
 **
 **  HISTORY    :
 **    06/03 Originated by Michael Halcrow <mike@halcrow.us>
 **    06/03 Furthered by Dustin Kirkland (k1rkland@us.ibm.com)
 **    10/03 Extended to invoke EACCES errno by Michael A. Halcrow
 **          <mike@halcrow.us>
 **    03/04 Added exp_errno variable by D. Kent Soper <dksoper@us.ibm.com>
 **    05/04 Updates to suppress compile warnings by Kimberly D. Simon <kdsimon@us.ibm.com>
 **
 **********************************************************************/
   
#include "syscalls.h"
#include <sys/types.h>
#include <sys/ipc.h>
#if defined(__PPC64)
#include <asm-ppc64/ipc.h>
#else
#include <asm/ipc.h>
#endif
#include <sys/msg.h>
   
int test_msgget(laus_data* dataPtr) {
    
  int rc = 0;
  int exp_errno = EACCES;
  int key = 0;
  int firstMsgid = 0;
  int secondMsgid = 0;
  //int doNotDeallocate = 0;   // not needed?
  int mode;
   
  // Set the syscall-specific data
  dataPtr->laus_var_data.syscallData.code = AUDIT_msgget;

  // Set the mode flags
  mode = S_IRWXU | S_IRWXG | S_IRWXO;
   
  // Set the key value.
  // If successCase == 0, then we will be double-allocating the memory
  // to force an error condition. 
  if( dataPtr->successCase ) {
    key = IPC_PRIVATE;
  } else {    
    mode = 0600 | IPC_CREAT;
    key = -1;
    if( ( firstMsgid = msgget( key, mode ) ) == -1 ) {
      printf1( "Cannot create the message queue with key = -1: errno = [%i]\n", errno );
      goto EXIT;
    }
  } 
   
  // Set up audit argument buffer
  if( ( rc = auditArg2( dataPtr,
		      AUDIT_ARG_IMMEDIATE, sizeof( int ), &key,
		      AUDIT_ARG_IMMEDIATE, sizeof( int ), &mode ) ) != 0 ) {
    printf1( "Error setting up audit argument buffer\n" );
    goto EXIT_CLEANUP;
  }

   
  // Do pre-system call work
  if ( (rc = preSysCall( dataPtr )) != 0 ) {
    printf1("ERROR: pre-syscall setup failed (%d)\n", rc);
    goto EXIT_CLEANUP;
  }
   
  // Execute system call
  //     dataPtr->laus_var_data.syscallData.result = secondMsgid = msgget( key, mode );
#if defined(__X86_64) && !defined(__MODE_32)
  dataPtr->laus_var_data.syscallData.result = secondMsgid = syscall( __NR_msgget, key, mode );
#else
  dataPtr->laus_var_data.syscallData.result = secondMsgid = syscall( __NR_ipc, MSGGET, key, mode );
#endif   
  // Do post-system call work
  if ( (rc = postSysCall(  dataPtr, errno, -1, exp_errno  )) != 0 ) {
    printf1("ERROR: post-syscall setup failed (%d)\n", rc);
    goto EXIT_CLEANUP;
  } 
   
 EXIT_CLEANUP:

  if( firstMsgid && ( firstMsgid != -1 ) ) {
    if( ( msgctl( firstMsgid, IPC_RMID, 0 ) ) == -1 ) {
      printf1( "ERROR: Cannot deallocate message memory with msgid=%d: errno=%i\n", 
	       firstMsgid, errno );
    }
  }    
  if( secondMsgid && ( secondMsgid != -1 ) ) {
    if( ( msgctl( secondMsgid, IPC_RMID, 0 ) ) == -1 ) {
      printf1( "ERROR: Cannot deallocate message memory with msgid=%d: errno=%i\n", 
	       secondMsgid, errno );
      goto EXIT;
    }
  }
         
 EXIT:
  return rc;
}
