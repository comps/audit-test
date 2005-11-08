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
 **  FILE       : test_tkill.c
 **
 **  PURPOSE    : To test the tkill library call auditing.
 **
 **  DESCRIPTION: The test_tkill() function builds into the
 **  laus_test framework to verify that the Linux Audit System
 **  accurately logs both successful and erroneous execution of the
 **  "tkill" system call.
 **
 **  In the successful case, this function:
 **   1) Forks a child process (which spins in a sleep loop)
 **   2) Calls the tkill() function on the child process
 **   3) Verifies the success result.
 **
 **  The successful case guarantees a success result from tkill() by
 **  maintaining superuser privileges while sending the SIGTKILL signal
 **  to the child process.
 **  
 **  In the erroneous case, this function:
 **   1) Calls the tkill() function on the current process group with
 **      the signal set to ``-1''
 **   2) Verifies the error result.
 **      
 **  The erroneous case guarantees an error result by sending ``-1''
 **  for the signal, which is an invalid value according to the man(7)
 **  page for signal.
 **
 **  HISTORY    :
 **    07/03 Originated by Michael A. Halcrow <mike@halcrow.us>
 **    03/04 Added exp_errno variable by D. Kent Soper <dksoper@us.ibm.com>
 **
 **********************************************************************/

#include "includes.h"
#include "syscalls.h"
#include <signal.h>

int test_tkill(laus_data *dataPtr)
{

    int rc = 0;
    int exp_errno = EINVAL;
    int pid = 0;
    int sig = -1;

    // Set the syscall-specific data
    printf5("Setting laus_var_data.syscallData.code to %d\n", AUDIT_tkill);
    dataPtr->laus_var_data.syscallData.code = AUDIT_tkill;

  /**
   * Do as much setup work as possible right here
   */
    if (dataPtr->successCase) {
	// Set up for success
	dataPtr->msg_euid = 0;
	dataPtr->msg_egid = 0;
	dataPtr->msg_fsuid = 0;
	dataPtr->msg_fsgid = 0;
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
    } else {
	// Set up for error
    }

    // Set up audit argument buffer
    if ((rc = auditArg2(dataPtr,
			AUDIT_ARG_IMMEDIATE, sizeof(int), &pid,
			AUDIT_ARG_IMMEDIATE, sizeof(int), &sig)) != 0) {
	printf1("Error setting up audit argument buffer\n");
	goto EXIT_CLEANUP;
    }
    // Do pre-system call work
    if ((rc = preSysCall(dataPtr)) != 0) {
	printf1("ERROR: pre-syscall setup failed (%d)\n", rc);
	goto EXIT_CLEANUP;
    }
    // Execute system call
    dataPtr->laus_var_data.syscallData.result = syscall(__NR_tkill, pid, sig);

    // Do post-system call work
    if ((rc = postSysCall(dataPtr, errno, -1, exp_errno)) != 0) {
	printf1("ERROR: post-syscall setup failed (%d)\n", rc);
	goto EXIT_CLEANUP;
    }

EXIT:
    printf5("Returning from test\n");
    return rc;

EXIT_CLEANUP:
  /**
   * Do cleanup work here
   */
    if (dataPtr->successCase) {
	// Clean up from success case setup
	if (kill(pid, SIGKILL) == -1) {
	    printf("Error killing process %d\n", pid);
	    goto EXIT;
	}
    }
    goto EXIT;
}
