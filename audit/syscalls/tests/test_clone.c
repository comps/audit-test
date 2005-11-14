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
 **  FILE       : test_clone.c
 **
 **  PURPOSE    : To test the clone library call auditing.
 **
 **  DESCRIPTION: The test_clone() function builds into the
 **  laus_test framework to verify that the Linux Audit System
 **  accurately logs both successful and erroneous execution of the
 **  "clone" system call.
 **
 **  In the successful case, this function:
 **   1) Malloc some memory for the child process stack
 **   2) Preform the syscall(__NR_clone ...  with the VFORK flag so 
 **      the parent waits.
 **   3) Clean up by freeing the child stack.
 **
 **  The successful case does a syscall(__NR_clone ...in the VFORK 
 **  mode.  The parent will wait and then finish the post_syscall 
 **  and clean-up stuff.
 **
 **  In the erroneous case, this function:
 **   1) Preforms the syscall(__NR_clone ... with the VFORK flag and 
 **      the CLONE_PID a NULL value for the stack.  This will only 
 **      fail if user is NOT root.
 **
 **  The erroneous case causes a failure by passing the CLONE_NEWNS flag.
 **  The error is EPERM(1).
 **
 **  HISTORY    :
 **    06/03 Originated by Kylene J. Smith <kylene@us.ibm.com>
 **    03/04 Added exp_errno variable by D. Kent Soper <dksoper@us.ibm.com>
 **    04/04 Modified test to call glibc interface by Kimberly D. Simon <kdsimon@us.ibm.com>
 **
 **********************************************************************/

#include "includes.h"
#include "syscalls.h"
#include <sched.h>

#ifndef __IA64
int fn(void *x)
{
    sleep(1);
    return 0;
}
#endif

int test_clone(struct audit_data *context)
{
    int rc = 0;
    int exp_errno = EPERM;
    int flags = CLONE_VFORK;
    // int flags = CLONE_FS|CLONE_VFORK|CLONE_PARENT|CLONE_SYSVSEM|0x8000068;
    pid_t pid;
    char *stack = NULL;

    // Do as much setup work as possible right here
    if (context->success) {	// Set up for success

    } else {			// Set up for error

	// BUGBUG: For some reason, we are not able to fail the the clone() syscall
	//   in the 2.6 kernel.
//              flags |= CLONE_NEWNS;
	rc = SKIP_TEST_CASE;
	goto EXIT;

    }

    // Set up audit argument buffer
    // Only save off first argument to assist log verification
    if ((rc =
	 auditArg1(context, AUDIT_ARG_IMMEDIATE, sizeof(int), &flags)) != 0) {
	fprintf(stderr, "Error setting up audit argument buffer\n");
	goto EXIT_CLEANUP;
    }
    // Do pre-system call work
    if ((rc = preSysCall(context)) != 0) {
	fprintf(stderr, "ERROR: pre-syscall setup failed (%d)\n", rc);
	goto EXIT_CLEANUP;
    }
    // Execute system call--parent waits b/c of CLONE_VFORK flag
#ifdef __IA64
    // ia64 glibc doesn't have a symbol for clone
    pid = syscall(__NR_clone, flags, NULL);
#else
    stack = malloc(65536);
    pid = clone(fn, (void *)(stack + 32768), flags, NULL);
#endif
    switch (pid) {
	case -1:
	    fprintf(stderr, "ERROR: clone failed (%d)\n", pid);
	    goto EXIT_CLEANUP;
	case 0:
	    //In child
	    _exit(0);
	default:
	    //In parent
	    context->u.syscall.exit = pid;
    }

    // Do post-system call work
    if ((rc = postSysCall(context, errno, -1, exp_errno)) != 0) {
	fprintf(stderr, "ERROR: post-syscall setup failed (%d)\n", rc);
	goto EXIT_CLEANUP;
    }


EXIT_CLEANUP:

EXIT:
    if (stack) {
	free(stack);
    }

    fprintf(stderr, "Returning from test\n");
    return rc;
}
