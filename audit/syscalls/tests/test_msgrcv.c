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
 **  FILE       : test_msgrcv.c
 **
 **  PURPOSE    : To test the msgrcv library call auditing.
 **
 **  DESCRIPTION: The test_msgrcv() function builds into the laus_test
 **  framework to verify that the Linux Audit System accurately logs
 **  both successful and erroneous execution of the "msgrcv" system call.
 **
 **  In the successful case, this function:
 **   1) Allocates a new message queue via msgget()
 **   2) Uses msgrcv to send a message
 **   3) Tests the result of the call against the expected successful
 **      return.
 **  
 **  In the erroneous case, this function:
 **   1) Uses msgrcv() to attempt to send a message with insufficient
 **      access permissions
 **   2) Tests the result of the call against the expected erroneous
 **      return.
 **
 **  HISTORY    :
 **    06/03 Originated by Michael Halcrow <mike@halcrow.us>
 **    06/03 Furthered by Dustin Kirkland  (k1rkland@us.ibm.com)
 **    10/03 Extended to invoke EPERM errno by Michael A. Halcrow
 **          <mike@halcrow.us>
 **    10/03 Modified by Dustin Kirkland (k1rkland@us.ibm.com) 
 **    03/04 Added exp_errno variable by D. Kent Soper <dksoper@us.ibm.com>
 **    04/04 Modified size of AUDIT_ARG_POINTER by Kimberly D. Simon <kdsimon@.us.ibm.com>
 **
 **********************************************************************/
   
#include "includes.h"
#include "syscalls.h"
#include <sys/ipc.h>
#include <sys/msg.h>
#if defined(__PPC64)
#include <asm-ppc64/ipc.h>
#else
#include <asm/ipc.h>
#endif
//#include <linux/ipc.h>
//#include <linux/msg.h>

// BUGBUG: remove for now.
//#define TEST_MSGRCV_VER0 


/* BUGBUG: Don't know what this is here for.
#ifdef TEST_MSGRCV_VER0
_syscall5(int, ipc, int, minor, int, msgqid, size_t, msgsz, int, msgflg, struct ipc_kludge *, ptr);
#else
_syscall6(int, ipc, int, minor, int, msgqid, size_t, msgsz, int, msgflg, struct msgbuf *, msgp, int, msgtyp);
#endif
*/

int test_msgrcv(laus_data* dataPtr) {
    
    
  int rc = 0;
  int exp_errno = EACCES;
  int msgid;
  int msgsz;
  int msgflg;
  long msgtyp;
  int mode;
  struct msgbuf {
    long mtype;
    char mtext[1];
  } buf1, buf2;
#ifdef TEST_MSGRCV_VER0
  struct ipc_kludge kludge;
#endif



  // Set the syscall-specific data
  dataPtr->laus_var_data.syscallData.code = AUDIT_msgrcv;

  mode = S_IRWXU;
  //msgsz = 1;
  msgsz = 3;
//Specifying MSG_EXCEPT and a type of 1 means get me all messages except those of
//type 1. As the only msg in the queue _has_ type 1 we get -ENOMSG;
//msgflg = IPC_NOWAIT | MSG_NOERROR | MSG_EXCEPT;
  msgflg = IPC_NOWAIT | MSG_NOERROR;
  msgtyp = 1;
  buf1.mtype = 1;
  buf1.mtext[0] = 'a';
  buf1.mtext[1] = 'b';
  buf1.mtext[2] = 'c';

  if( ( msgid = msgget( IPC_PRIVATE, mode ) ) == -1 ) {
    printf1( "ERROR: Unable to allocate new message queue: errno=%i\n", errno );
    goto EXIT;
  }

  if ( msgsnd( msgid, &buf1, msgsz, msgflg ) == -1 ) {
    printf1( "ERROR: Unable to add a message to the message queue: errno=%i\n", errno );
    goto EXIT_CLEANUP;
  }

  if( dataPtr->successCase ) {
    dataPtr->msg_euid = dataPtr->msg_ruid = dataPtr->msg_fsuid = 0;
  } 
   
  // Set up audit argument buffer
  if( ( rc = auditArg5( dataPtr,
			AUDIT_ARG_IMMEDIATE, sizeof( int ), &msgid,
                        AUDIT_ARG_POINTER, 0, &buf2,
                        AUDIT_ARG_IMMEDIATE, sizeof( int ), &msgsz,
                        AUDIT_ARG_IMMEDIATE, sizeof( long ), &msgtyp,
                        AUDIT_ARG_IMMEDIATE, sizeof( int ), &msgflg
			) ) != 0 ) {
    printf1( "Error setting up audit argument buffer\n" );
    goto EXIT_CLEANUP;
  }
   
  // Do pre-system call work
  if ( (rc = preSysCall( dataPtr )) != 0 ) {
    printf1("ERROR: pre-syscall setup failed (%d)\n", rc);
    goto EXIT_CLEANUP;
  }
   
  // Execute system call
  dataPtr->laus_var_data.syscallData.result = msgrcv( msgid, &buf2, msgsz, msgtyp, msgflg ); 
  /*
#ifdef __X86_64
  dataPtr->laus_var_data.syscallData.result = syscall( __NR_msgrcv, msgid, msgsz, msgflg, &buf2, msgtyp ); 
#elif defined(TEST_MSGRCV_VER0)
	kludge.msgp = &buf2;
	kludge.msgtyp = msgtyp;
	dataPtr->laus_var_data.syscallData.result = syscall( __NR_ipc, MSGRCV, msgid, msgsz, msgflg, &kludge ); 
#else
	dataPtr->laus_var_data.syscallData.result = syscall( __NR_ipc, 0x10000|MSGRCV, msgid, msgsz, 
							     msgflg, &buf2, msgtyp ); 
#endif
  */
  // Do post-system call work
  if ( (rc = postSysCall(  dataPtr, errno, -1, exp_errno )) != 0 ) {
    printf1("ERROR: post-syscall setup failed (%d)\n", rc);
    goto EXIT_CLEANUP;
  }
   
  
 EXIT_CLEANUP:

  if( msgid && ( msgid != -1 ) ) {
    if( ( rc = msgctl( msgid, IPC_RMID, 0 ) ) == -1 ) {
      printf1( "Error removing message queue with ID = [%d]: errno = [%i]\n", msgid, errno );
      goto EXIT;
    }
  }
   
 EXIT:
  return rc;
}
   
