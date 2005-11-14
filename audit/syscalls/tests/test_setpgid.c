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
 **  FILE       : test_setpgid.c
 **
 **  PURPOSE    : To test the setpgid library call auditing.
 **
 **  DESCRIPTION: The test_setpgid() function builds into the
 **  laus_test framework to verify that the Linux Audit System
 **  accurately logs both successful and erroneous execution of the
 **  "setpgid" system call.
 **
 **  In the successful case, this function:
 **   1) Forks to child
 **   1) Calls setpgid with pid=0 and pgid=0
 **   2) Verifies the success result.
 **
 **  The successful case does not change the current settings for the
 **  current process; it just assures that the setpgid call succeeds.
 **  
 **  In the erroneous case, this function:
 **   1) Calls setpgid with pid=0 and pgid=-1
 **   2) Verifies the error result.
 **      
 **  The erroneous case causes an error condition in setpgid by
 **  passing in a pgid value that is less than 0, which, according to
 **  the man page, results in the error code EINVAL.
 **
 **  HISTORY    :
 **    06/03 Originated by Michael A. Halcrow <mikes@halcrow.us>
 **    03/04 Added exp_errno variable by D. Kent Soper <dksoper@us.ibm.com>
 **
 **********************************************************************/

#include "includes.h"
#include "syscalls.h"

int test_setpgid(struct audit_data *context)
{
    int rc = 0;
    int exp_errno = EINVAL;

    int pid;
    int pgid;

  /**
   * Do as much setup work as possible right here
   */
    context->euid = 0;
    context->egid = 0;
    context->fsuid = 0;
    context->fsgid = 0;
    if (context->success) {
	// Set up for success
	pid = 0;
	pgid = 0;
    } else {
	// Set up for error
	pid = 0;
	pgid = -1;
    }

    // Set up audit argument buffer
    if ((rc = auditArg2(context,
			AUDIT_ARG_IMMEDIATE, sizeof(int), &pid,
			AUDIT_ARG_IMMEDIATE, sizeof(int), &pgid)) != 0) {
	fprintf(stderr, "Error setting up audit argument buffer\n");
	goto EXIT;
    }
    // Do pre-system call work
    if ((rc = preSysCall(context)) != 0) {
	fprintf(stderr, "ERROR: pre-syscall setup failed (%d)\n", rc);
	goto EXIT_CLEANUP;
    }
    // Execute system call
    context->u.syscall.exit = syscall(__NR_setpgid, pid, pgid);

    // Do post-system call work
    if ((rc = postSysCall(context, errno, -1, exp_errno)) != 0) {
	fprintf(stderr, "ERROR: post-syscall setup failed (%d)\n", rc);
	goto EXIT_CLEANUP;
    }

EXIT_CLEANUP:
  /**
   * Do cleanup work here
   */
    if (context->success) {
	// Clean up from success case setup
    }

EXIT:
    fprintf(stderr, "Returning from test\n");
    return rc;
}
