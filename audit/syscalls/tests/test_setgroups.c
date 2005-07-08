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
    **  FILE       : test_setgroups.c
    **
    **  PURPOSE    : To test the setgroups library call auditing.
    **
    **  DESCRIPTION: The test_setgroups() function builds into the
    **  laus_test framework to verify that the Linux Audit System
    **  accurately logs both successful and erroneous execution of the
    **  "setgroups" system call.
    **
    **  In the successful case, this function:
    **   1) Executes the setgroups syscall with size=1 and list={0}
    **   3) Verifies success by verifying that setgroups() did not return
    **      a -1 result.
    **
    **  The successful case provides a valid address for list and sets
    **  list to be just one item long.  It runs setgroups() as the
    **  superuser, and so according to the man page, all requirements are
    **  met for a successful result.
    **  
    **  In the erroneous case, this function:
    **   1) Sets the euid to the test user
    **   2) Attempts to execute the setgroups syscall
    **   3) Sets the euid to the superuser
    **   4) Verifies error by verifying that setgroups() returned a -1
    **      result.
    **      
    **  The erroneous case forces an error by attempting to run
    **  setgroups() as someone other than the root user (assuming the
    **  test user is not root, which is a valid assumption for this suite
    **  of audit tests).  According to the man page, only the superuser
    **  may use the setgroups() function.
    **
    **  HISTORY    :
    **    06/03 Originated by Michael A. Halcrow <mike@halcrow.us>
    **    03/04 Added exp_errno variable by D. Kent Soper <dksoper@us.ibm.com>
    **    05/04 Updates to suppress compile warnings by Kimberly D. Simon <kdsimon@us.ibm.com>
    **
    **********************************************************************/

#include "includes.h"
#include "syscalls.h"
#include <grp.h>

int test_setgroups(laus_data *dataPtr)
{


    int rc = 0;
    int exp_errno = EPERM;

    size_t size = 1;
    gid_t list[1] = { 0 };
    //on 32 bit platforms the gid in the kernel is 16 bits (i.e. logged as such)
    //u_int16_t log_list[1] = { 0 };           //not needed?

    // Set the syscall-specific data
    printf5("Setting laus_var_data.syscallData.code to %d\n", AUDIT_setgroups);
    dataPtr->laus_var_data.syscallData.code = AUDIT_setgroups;

     /**
      * Do as much setup work as possible right here
      */
    if (dataPtr->successCase) {
	dataPtr->msg_euid = 0;
	dataPtr->msg_egid = 0;
	dataPtr->msg_fsuid = 0;
	dataPtr->msg_fsgid = 0;
	// Set up audit argument buffer for success case
	if ((rc = auditArg1(dataPtr,
			    AUDIT_ARG_POINTER, sizeof(gid_t), list)) != 0) {
	    printf1("Error setting up audit argument buffer\n");
	    goto EXIT;
	}
    } else {
	// Set up audit argument buffer for fail case
	if ((rc = auditArg1(dataPtr, AUDIT_ARG_NULL, 0, NULL)) != 0) {
	    printf1("Error setting up audit argument buffer\n");
	    goto EXIT;
	}
    }


    // Do pre-system call work  
    if ((rc = preSysCall(dataPtr)) != 0) {
	printf1("ERROR: pre-syscall setup failed (%d)\n", rc);
	goto EXIT;
    }

    dataPtr->laus_var_data.syscallData.result =
	syscall(__NR_setgroups, size, &list);

    // Do post-system call work
    if ((rc = postSysCall(dataPtr, errno, -1, exp_errno)) != 0) {
	printf1("ERROR: post-syscall setup failed (%d)\n", rc);
	goto EXIT;
    }

EXIT:
    printf5("Returning from test\n");
    return rc;
}
