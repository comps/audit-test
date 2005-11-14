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
 **  FILE       : test_kill.c
 **
 **  PURPOSE    : To test the kill library call auditing.
 **
 **  DESCRIPTION: The test_kill() function builds into the
 **  laus_test framework to verify that the Linux Audit System
 **  accurately logs both successful and erroneous execution of the
 **  "kill" system call.
 **
 **  In the successful case, this function:
 **   1) Forks a child process (which spins in a sleep loop)
 **   2) Calls the kill() function on the child process
 **   3) Verifies the success result.
 **
 **  The successful case guarantees a success result from kill() by
 **  maintaining superuser privileges while sending the SIGKILL signal
 **  to the child process.
 **  
 **  In the erroneous case, this function:
 **   1) Attempts to kill a process by a non-root, non-owner
 **   
 **  The erroneous case guarantees an error by trying to kill a process
 **  by a user other than root or the owner of the process
 **
 **  HISTORY    :
 **    06/03 Originated by Michael A. Halcrow <mike@halcrow.us>
 **    09/03 Furthered by Dustin Kirkland <k1rkland@us.ibm.com>
 **    03/04 Added exp_errno variable by D. Kent Soper <dksoper@us.ibm.com>
 **
 **********************************************************************/

#include "includes.h"
#include "syscalls.h"
#include <signal.h>

int test_kill(struct audit_data *context)
{

    int rc = 0;
    int exp_errno = EPERM;

    int pid = 0;
    int sig = -1;

  /**
   * Do as much setup work as possible right here
   */
    if (context->success) {
	// Set up for success
	context->euid = 0;
	context->egid = 0;
	context->fsuid = 0;
	context->fsgid = 0;
    } else {
	// do not become root
	context->euid = context->fsuid = helper_uid;
    }
    // Spawn the child process
    printf5("Spawning the child\n");
    if ((pid = fork()) == 0) {
	printf5("I am the child!\n");
	// This is the child
	while (1) {
	    sleep(1);
	}
    }
    printf5("I am the parent!\n");
    sig = SIGKILL;

    // Set up audit argument buffer
    if ((rc = auditArg2(context,
			AUDIT_ARG_IMMEDIATE, sizeof(int), &pid,
			AUDIT_ARG_IMMEDIATE, sizeof(int), &sig)) != 0) {
	printf1("Error setting up audit argument buffer\n");
	goto EXIT_CLEANUP;
    }
    // Do pre-system call work
    if ((rc = preSysCall(context)) != 0) {
	printf1("ERROR: pre-syscall setup failed (%d)\n", rc);
	goto EXIT_CLEANUP;
    }
    // Execute system call
    context->u.syscall.exit = syscall(__NR_kill, pid, sig);

    // Do post-system call work
    if ((rc = postSysCall(context, errno, -1, exp_errno)) != 0) {
	printf1("ERROR: post-syscall setup failed (%d)\n", rc);
	goto EXIT_CLEANUP;
    }

EXIT_CLEANUP:
  /**
   * Do cleanup work here
   */
    if ((pid) && !(context->success)) {
	if (kill(pid, SIGKILL) == -1) {
	    printf("Error kill process %d\n", pid);
	}
    }

    printf5("Returning from test\n");
    return rc;
}
