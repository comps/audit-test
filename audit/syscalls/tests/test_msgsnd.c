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
 **  FILE       : test_msgsnd.c
 **
 **  PURPOSE    : To test the msgsnd library call auditing.
 **
 **  DESCRIPTION: The test_msgsnd() function builds into the laus_test
 **  framework to verify that the Linux Audit System accurately logs
 **  both successful and erroneous execution of the "msgsnd" system call.
 **
 **  In the successful case, this function:
 **   1) Allocates a new message queue via msgget()
 **   2) Uses msgsnd to send a message
 **   3) Tests the result of the call against the expected successful
 **      return.
 **  
 **  In the erroneous case, this function:
 **   1) Uses msgsnd() to attempt to send a message with insufficient
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
 **    04/04 Increased msgsz variable and size for AUDIT_ARG_POINTER for 32-bit and 64-bit,
 ** 	     modified #ifndef by Kimberly D. Simon <kdsimon@us.ibm.com>
 **
 **********************************************************************/

#include "includes.h"
#include "syscalls.h"
#include <sys/ipc.h>
#include <sys/msg.h>
#if defined(__PPC64)
#include <asm-ppc64/ipc.h>
#elif !defined(__IA64)
#include <asm/ipc.h>
#endif

int test_msgsnd(struct audit_data *context)
{
    int rc = 0;
    int exp_errno = EACCES;
    int msgid = 0;
    int msgsz;
    int msgflg;
    int mode;
    struct msgbuf {
	long mtype;
	char mtext[10];
    } buf;

    mode = S_IRWXU;
    if ((msgid = msgget(IPC_PRIVATE, mode)) == -1) {
	fprintf(stderr, "ERROR: Unable to allocate new message queue: errno=%i\n",
		errno);
	goto EXIT;
    }

    msgsz = 3;			// Determines how much data will actually be in the msgbuf structure
    msgflg = IPC_NOWAIT;
    buf.mtype = 1;
    memset(buf.mtext, '\0', sizeof(buf.mtext));
    buf.mtext[0] = 'a';
    buf.mtext[1] = 'b';
    buf.mtext[2] = 'c';


    printf(" >>> buf address: %p <<< \n", &buf);
    if (context->success) {
	context->euid = context->fsuid = 0;
    } else {
    }
    // Set up audit argument buffer
    if ((rc = auditArg4(context, AUDIT_ARG_IMMEDIATE, sizeof(int), &msgid,
			AUDIT_ARG_POINTER, sizeof(long) + msgsz, &buf,
			AUDIT_ARG_IMMEDIATE, sizeof(int), &msgsz,
			AUDIT_ARG_IMMEDIATE, sizeof(int), &msgflg)) != 0) {
	fprintf(stderr, "Error setting up audit argument buffer\n");
	goto EXIT_CLEANUP;
    }
    // Do pre-system call work
    if ((rc = preSysCall(context)) != 0) {
	fprintf(stderr, "ERROR: pre-syscall setup failed (%d)\n", rc);
	goto EXIT_CLEANUP;
    }
    // Execute system call
#if (defined(__X86_64) || defined(__IA64)) && !defined(__MODE_32)
    context->u.syscall.exit =
	syscall(__NR_msgsnd, msgid, &buf, msgsz, msgflg);
#else
    context->u.syscall.exit =
	syscall(__NR_ipc, MSGSND, msgid, msgsz, msgflg, &buf);
#endif

    //  context->u.syscall.exit = msgsnd( msgid, &buf, msgsz, msgflg ); 

    // Do post-system call work
    if ((rc = postSysCall(context, errno, -1, exp_errno)) != 0) {
	fprintf(stderr, "ERROR: post-syscall setup failed (%d)\n", rc);
	goto EXIT_CLEANUP;
    }


EXIT_CLEANUP:

    if (!context->success && msgid && (msgid != -1)) {
	if ((rc = msgctl(msgid, IPC_RMID, 0)) == -1) {
	    fprintf
		(stderr,
		 "Error removing message queue with ID = [%d]: errno = [%i]\n",
		 msgid, errno);
	    goto EXIT;
	}
    }

EXIT:
    return rc;
}
