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
 **  FILE       : test_msgctl.c
 **
 **  PURPOSE    : To test the msgctl library call auditing.
 **
 **  DESCRIPTION: The test_msgctl() function builds into the laus_test
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
 **
 **  Message queue operations are attempted as a non-root user, thus
 **  causing an EPERM errno.
 **
 **  HISTORY    :
 **    06/03 Originated by Michael Halcrow <mike@halcrow.us>
 **    06/03 Furthered by Dustin Kirkland  (k1rkland@us.ibm.com)
 **    10/03 Extended to invoke EPERM errno by Michael A. Halcrow
 **          <mike@halcrow.us>
 **    03/04 Added exp_errno variable by D. Kent Soper <dksoper@us.ibm.com>
 **    04/04 Updated test for 2.6 LAuS implementation by Kimberly D. Simon <kdsimon@us.ibm.com>
 **    05/04 Updates to suppress compile warnings by Kimberly D. Simon <kdsimon@us.ibm.com>
 **
 **********************************************************************/

#include "includes.h"
#include "syscalls.h"
#include <sys/ipc.h>
#include <sys/msg.h>
#if defined(__powerpc64__)
#include <asm-ppc64/ipc.h>
#elif !defined(__ia64__)
#include <asm/ipc.h>
#endif

int test_msgctl(struct audit_data *context)
{
    int rc = 0;
    int exp_errno = -EPERM;
    int msgid = 0;
    int mode;
    static int cmd = IPC_RMID;
    struct msqid_ds buf;

    memset(&buf, 0, sizeof(buf));

    // Allocate shared memory space so that we can test deallocation via msgctl
    mode = S_IRWXU;
    if ((msgid = msgget(IPC_PRIVATE, mode)) == -1) {
	fprintf(stderr, "ERROR: Unable to allocate new message queue: errno=%i\n",
		errno);
	goto EXIT;
    }
    if (context->success) {
	context->euid = 0;
	context->egid = 0;
	context->fsuid = 0;
	context->fsgid = 0;
    } else {
    }

    // Set up audit argument buffer
    if ((rc = auditArg3(context,
			AUDIT_ARG_IMMEDIATE, sizeof(int), &msgid,
			AUDIT_ARG_IMMEDIATE, sizeof(int), &cmd,
			AUDIT_ARG_POINTER, 0, &buf)) != 0) {
	fprintf(stderr, "Error setting up audit argument buffer\n");
	goto EXIT_CLEANUP;
    }
    // Do pre-system call work
    if ((rc = preSysCall(context)) != 0) {
	fprintf(stderr, "ERROR: pre-syscall setup failed (%d)\n", rc);
	goto EXIT_CLEANUP;
    }
    // Execute system call
#if (defined(__x86_64__) || defined(__ia64__))
    context->u.syscall.exit = syscall(__NR_msgctl, msgid, cmd, &buf);
#else
    context->u.syscall.exit = syscall(__NR_ipc, MSGCTL, 
					msgid, cmd, 0, &buf);
#endif
    // Do post-system call work
    if ((rc = postSysCall(context, errno, -1, exp_errno)) != 0) {
	fprintf(stderr, "ERROR: post-syscall setup failed (%d)\n", rc);
	goto EXIT_CLEANUP;
    }


EXIT_CLEANUP:

    if (!context->success && msgid && (msgid != -1)) {
	if ((rc = msgctl(msgid, cmd, 0)) == -1) {
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
