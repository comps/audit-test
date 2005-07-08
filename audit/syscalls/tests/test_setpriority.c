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
    **  FILE       : test_setpriority.c
    **
    **  PURPOSE    : To test the setpriority library call auditing.
    **
    **  DESCRIPTION: The test_setpriority() function builds into the
    **  laus_test framework to verify that the Linux Audit System
    **  accurately logs both successful and erroneous execution of the
    **  "setpriority" system call.
    **
    **  In the successful case, this function:
    **   1) Calls setpriority on the current process with a target
    **      priority value of 1
    **   2) Verifies a successful result.
    **
    **  The successful case sets all the parameters for the setpriority
    **  call to valid values.  We execute the syscall as the superuser to
    **  guarantee success.
    **  
    **  In the erroneous case, this function:
    **   1) Calls setpriority with an invalid ``which'' value
    **   2) Verifies an erroneous result.
    **      
    **  The erroneous case calls setpriority with which set to a value
    **  other than that which is listed in the man page as a valid
    **  value.  Hence, we can expect an error result.
    **
    **  HISTORY    :
    **    06/03 Originated by Michael A. Halcrow <mike@halcrow.us>
    **    03/04 Added exp_errno variable by D. Kent Soper <dksoper@us.ibm.com>
    **
    **********************************************************************/

#include "includes.h"
#include "syscalls.h"
#include <sys/time.h>
#include <sys/resource.h>

int test_setpriority(laus_data *dataPtr)
{


    int rc = 0;
    int exp_errno = EINVAL;

    int which;
    int who;
    int prio;

    // Set the syscall-specific data
    printf5("Setting laus_var_data.syscallData.code to %d\n",
	    AUDIT_setpriority);
    dataPtr->laus_var_data.syscallData.code = AUDIT_setpriority;

     /**
      * Do as much setup work as possible right here
      */
    if (dataPtr->successCase) {
	// Set up for success
	dataPtr->msg_euid = 0;
	dataPtr->msg_egid = 0;
	dataPtr->msg_fsuid = 0;
	dataPtr->msg_fsgid = 0;
	which = PRIO_PROCESS;
	who = 0;
	prio = 1;
    } else {
	// Set up for error
	which = 42;
	while (which == PRIO_PROCESS || which == PRIO_PGRP
	       || which == PRIO_USER) {
	    which++;
	}
	who = 0;
	prio = 1;
    }

    // Set up audit argument buffer
    //auditing only logging the first 3 args
    if ((rc = auditArg3(dataPtr,
			AUDIT_ARG_IMMEDIATE, sizeof(int), &which,
			AUDIT_ARG_IMMEDIATE, sizeof(int), &who,
			AUDIT_ARG_IMMEDIATE, sizeof(int), &prio)) != 0) {
	printf1("Error setting up audit argument buffer\n");
	goto EXIT;
    }
    // Do pre-system call work
    if ((rc = preSysCall(dataPtr)) != 0) {
	printf1("ERROR: pre-syscall setup failed (%d)\n", rc);
	goto EXIT_CLEANUP;
    }
    // Execute system call
    dataPtr->laus_var_data.syscallData.result =
	syscall(__NR_setpriority, which, who, prio);

    // Do post-system call work
    if ((rc = postSysCall(dataPtr, errno, -1, exp_errno)) != 0) {
	printf1("ERROR: post-syscall setup failed (%d)\n", rc);
	goto EXIT_CLEANUP;
    }

EXIT_CLEANUP:
     /**
      * Do cleanup work here
      */
    if (dataPtr->successCase) {
	// Clean up from success case setup
    }

EXIT:
    printf5("Returning from test\n");
    return rc;
}
