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
    **  FILE       : test_setresgid16.c
    **
    **  PURPOSE    : To test the setresgid16 library call auditing.
    **
    **  DESCRIPTION: The test_setresgid16() function builds into the
    **  laus_test framework to verify that the Linux Audit System
    **  accurately logs both successful and erroneous execution of the
    **  "setresgid16" system call.
    **
    **  In the successful case, this function:
    **   1) Clears the audit trail
    **   2) Calls setresgid16() with egid=-1, rgid=-1, and sgid=-1
    **   3) Verifies that the setresgid16 call executed successfully.
    **
    **  The successful case passes the setresgid16() parameters egid=-1,
    **  rgid=-1, sgid=-1.  According to the man page, this causes no
    **  action to be taken while the syscall returns with a successful
    **  result.
    **  
    **  In the erroneous case, this function:
    **   1) Sets the euid to the test user
    **   2) Discovers an egid that will result in a failure when passed
    **      to setresgid16() by the test user
    **   3) Sets the euid to the superuser
    **   4) Clears the audit trail
    **   5) Sets the euid to the test user
    **   6) Attempts to set the egid to the unique, invalid egid
    **      determined in step (2)
    **   7) Sets the euid to the superuser
    **   8) Verifies that setresgid16 call executed erroneously.
    **      
    **  The erroneous case satisfies the condition for failure as
    **  detailed in the man page.  It sets rgid and sgid to -1 to leave
    **  them unchanged, and it attempts to set egid to a unique, invalid
    **  value which will cause setresgid16() to return a failure code.
    **
    **  HISTORY    :
    **    06/03 Originated by Michael A. Halcrow <mike@halcrow.us>
    **    03/04 Added exp_errno variable by D. Kent Soper <dksoper@us.ibm.com>
    **    05/04 Updates to suppress compile warnings by Kimberly D. Simon <kdsimon@us.ibm.com>
    **
    **********************************************************************/
#if !defined(__PPC) && !defined(__X86_64)  && !defined(__IA64)
#include "includes.h"
#include "syscalls.h"

int test_setresgid16(struct audit_data *context)
{
    int rc = 0;
    int exp_errno = EPERM;
    int rgid, egid, sgid;


    // Set the syscall-specific data
    printf5("Setting u.syscall.sysnum to %d\n", AUDIT_setresgid);
    context->u.syscall.sysnum = AUDIT_setresgid;

     /**
      * Do as much setup work as possible right here
      */
    if (context->success) {
	egid = -1;
	rgid = -1;
	sgid = -1;
	printf5("Target egid=%d and rgid=%d in success case\n", egid, rgid);
	context->euid = 0;
	context->egid = 0;
	context->fsuid = 0;
	context->fsgid = 0;
    } else {
	identifiers_t identifiers;
       /**
        * To test the failure case, the following conditions must apply:
        *  - I am not the superuser
        *  - The new fsgid CANNOT match any one of the following:
        *   - real group ID
        *   - effective group ID
        *   - saved set-group-ID
        *   - current value of fsgid
        */
	// Pick a nice round ID, test it, and increment it on every
	// sequential failure until we find something that works
	egid = 42;
	rgid = -1;		// Just leave this unchanged
	sgid = -1;		// same here

	// su to test user
	printf5("seteuid to %i\n", context->euid);
	if ((rc = seteuid(context->euid)) != 0) {
	    printf1("Unable to seteuid to %i: errno=%i\n",
		    context->euid, errno);
	    goto EXIT;		// Or possibly EXIT_CLEANUP
	}

	if ((rc = getIdentifiers(&identifiers) != 0)) {
	    printf1("Utility getIdentifiers failed\n");
	    goto EXIT;
	}
	while (egid == identifiers.rgid || egid == identifiers.egid ||
	       egid == identifiers.sgid) {
	    egid++;
	}

	// su to superuser
	printf5("seteuid to root\n");
	if ((rc = seteuid(0)) != 0) {
	    printf1("Unable to seteuid to root: errno=%i\n", errno);
	    goto EXIT_CLEANUP;	// Or possibly EXIT_CLEANUP
	}

    }

    // Set up audit argument buffer
    if ((rc = auditArg3(context,
			AUDIT_ARG_IMMEDIATE_u, sizeof(rgid), &rgid,
			AUDIT_ARG_IMMEDIATE_u, sizeof(egid), &egid,
			AUDIT_ARG_IMMEDIATE_u, sizeof(sgid), &sgid)) != 0) {
	printf1("Error setting up audit argument buffer\n");
	goto EXIT;
    }
    // Do pre-system call work
    if ((rc = preSysCall(context)) != 0) {
	printf1("ERROR: pre-syscall setup failed (%d)\n", rc);
	goto EXIT_CLEANUP;
    }

    context->u.syscall.exit = syscall(__NR_setresgid16, rgid, egid, sgid);

    // Do post-system call work
    if ((rc = postSysCall(context, errno, -1, exp_errno)) != 0) {
	printf1("ERROR: post-syscall setup failed (%d)\n", rc);
	goto EXIT_CLEANUP;
    }

EXIT_CLEANUP:
     /**
      * Do cleanup work here
      */

EXIT:
    printf5("Returning from test\n");
    return rc;
}
#endif
