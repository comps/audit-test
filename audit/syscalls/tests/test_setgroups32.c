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
    **  FILE       : test_setgroups32.c
    **
    **  PURPOSE    : To test the setgroups32 library call auditing.
    **
    **  DESCRIPTION: The test_setgroups32() function builds into the
    **  laus_test framework to verify that the Linux Audit System
    **  accurately logs both successful and erroneous execution of the
    **  "setgroups32" system call.
    **
    **  In the successful case, this function:
    **   1) Executes the setgroups32 syscall with size=1 and list={0}
    **   3) Verifies success by verifying that setgroups32() did not return
    **      a -1 result.
    **
    **  The successful case provides a valid address for list and sets
    **  list to be just one item long.  It runs setgroups32() as the
    **  superuser, and so according to the man page, all requirements are
    **  met for a successful result.
    **  
    **  In the erroneous case, this function:
    **   1) Sets the euid to the test user
    **   2) Attempts to execute the setgroups32 syscall
    **   3) Sets the euid to the superuser
    **   4) Verifies error by verifying that setgroups32() returned a -1
    **      result.
    **      
    **  The erroneous case forces an error by attempting to run
    **  setgroups32() as someone other than the root user (assuming the
    **  test user is not root, which is a valid assumption for this suite
    **  of audit tests).  According to the man page, only the superuser
    **  may use the setgroups32() function.
    **
    **  HISTORY    :
    **    06/03 Originated by Michael A. Halcrow <mike@halcrow.us>
    **    03/04 Added exp_errno variable by D. Kent Soper <dksoper@us.ibm.com>
    **
    **********************************************************************/

#include "includes.h"
#include "syscalls.h"
#include <grp.h>

int test_setgroups32(struct audit_data *context)
{
    int rc = 0;
#if !defined(__PPC) && !defined(__X86_64) && !defined(__S390X) && !defined(__IA64)
    int exp_errno = EPERM;

    size_t size = 1;
    gid_t list[1] = { 0 };

     /**
      * Do as much setup work as possible right here
      */
    if (context->success) {
	context->euid = 0;
	context->egid = 0;
	context->fsuid = 0;
	context->fsgid = 0;
	// Set up audit argument buffer for success case
	if ((rc = auditArg1(context,
			    AUDIT_ARG_POINTER, sizeof(gid_t), list)) != 0) {
	    printf1("Error setting up audit argument buffer\n");
	    goto EXIT;
	}
    } else {
	// Set up audit argument buffer for fail case
	if ((rc = auditArg1(context, AUDIT_ARG_NULL, 0, NULL)) != 0) {
	    printf1("Error setting up audit argument buffer\n");
	    goto EXIT;
	}
    }


    // Do pre-system call work  
    if ((rc = preSysCall(context)) != 0) {
	printf1("ERROR: pre-syscall setup failed (%d)\n", rc);
	goto EXIT;
    }

    context->u.syscall.exit = syscall(__NR_setgroups32, size, list);

    // Do post-system call work
    if ((rc = postSysCall(context, errno, -1, exp_errno)) != 0) {
	printf1("ERROR: post-syscall setup failed (%d)\n", rc);
	goto EXIT;
    }

EXIT:
    printf5("Returning from test\n");
#endif
    return rc;
}
