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
 **  FILE       : test_msgctl_einval.c
 **
 **  PURPOSE    : To test the msgctl library call auditing.
 **
 **  DESCRIPTION: The test_msgctl_einval() function builds into the laus_test
 **  framework to verify that the Linux Audit System accurately logs
 **  both successful and erroneous execution of the "lpc" system call.
 **
 **  In the successful case, this function:
 **   1) Allocates a new message queue via msgget()
 **   2) Uses msgctl() to deallocate the newly allocated message queue
 **   3) Tests the result of the call against the expected successful
 **      return.
 **  
 **  The successful case uses the msgid returned by msgget() in using
 **  msgctl() to deallocate the memory.  If msgctl() returns 0, we have
 **  a success, as specified in the man page.
 **
 **  In the erroneous case, this function:
 **   1) Uses msgctl() to attempt to deallocate a message queue
 **      with the invalid error identifier (-1)
 **   2) Tests the result of the call against the expected erroneous
 **      return.
 **
 **  Since -1 is documented in msgget()'s man page as the msgid value
 **  returned when an error has occured, we infer that -1 is an invalid
 **  identifier.  In the erroneous case, we expect msgctl() to return
 **  -1 on error when given -1 as the msgid, and errno should be set to
 **  EINVAL, which, according to the man page, is the value set when
 **  msgid is not a valid identifier.
 **
 **  HISTORY    :
 **    06/03 Originated by Michael Halcrow <mike@halcrow.us>
 **    06/03 Furthered by Dustin Kirkland  (k1rkland@us.ibm.com)
 **    03/04 Added exp_errno variable by D. Kent Soper <dksoper@us.ibm.com>
 **
 **********************************************************************/
   
#include "syscalls.h"
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include "../../include/asm/ipc.h"
   
int test_msgctl_einval(laus_data* dataPtr) {
    
    
  int rc = 0;
  int exp_errno = EINVAL;
  //  int msgctlrc = 0;
  int msgid = 0;
  key_t key;
  int mode;
  static int cmd = IPC_RMID;
  struct msqid_ds buf;

  // Set the syscall-specific data
  dataPtr->laus_var_data.syscallData.code = AUDIT_msgctl;

  // su to test user
  printf5( "seteuid to %i\n", dataPtr->msg_euid );
  if( ( rc = seteuid( dataPtr->msg_euid ) ) != 0 ) {
    printf1( "ERROR: Unable to seteuid to %i: errno=%i\n", 
	     dataPtr->msg_euid, errno );
    goto EXIT;
  }
   
  memset(&buf, 0, sizeof(buf));

  if( dataPtr->successCase ) {
      // Allocate shared memory space so that we can test deallocation via msgctl
      mode = S_IRWXU | S_IRWXG | S_IRWXO;
      if( ( msgid = msgget( IPC_PRIVATE, mode ) ) == -1 ) {
	  printf1( "ERROR: Unable to allocate new message queue: errno=%i\n", errno );
	  goto EXIT;
      }
  } else {
      msgid = -1;
  }
   
  // su back to root
  printf5( "seteuid to root\n" );
  if ( ( rc = seteuid( 0 ) ) != 0 ) {
    printf1( "ERROR: Unable to seteuid to root: errno=%i\n", errno );
    goto EXIT;
  }
   
  // Set up audit argument buffer
  if( ( rc = auditArg3( dataPtr,
			AUDIT_ARG_IMMEDIATE, sizeof( int ), &msgid,
			AUDIT_ARG_IMMEDIATE, sizeof( int ), &cmd,
#if !defined(__PPC) && !defined(__X86_64) 			
			AUDIT_ARG_POINTER, 56, &buf
#else
			AUDIT_ARG_POINTER, 104, &buf
#endif
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
#ifndef __X86_64
  dataPtr->laus_var_data.syscallData.result = syscall( __NR_ipc,        // syscall
						       MSGCTL,    // name
						       msgid,           // First
						       IPC_RMID,        // Second
						       0,
						       &buf );           // Third
#else
  dataPtr->laus_var_data.syscallData.result = syscall( __NR_msgctl,        // syscall
						       msgid,           // First
						       IPC_RMID,        // Second
//						       0,
						       &buf );           // Third

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
   
