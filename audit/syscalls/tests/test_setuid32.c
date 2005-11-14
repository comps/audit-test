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
    **  FILE       : test_setuid32.c
    **
    **  PURPOSE    : To test the setuid32 library call auditing.
    **
    **  DESCRIPTION: The test_setuid32() function builds into the
    **  laus_test framework to verify that the Linux Audit System
    **  accurately logs both successful and erroneous execution of the
    **  "setuid32" system call.
    **
    **  In the successful case, this function:
    **   1) Clears the audit trail
    **   2) Calls setuid32() with uid=0
    **   3) Verifies that the setuid32 call executed successfully.
    **
    **  The successful case passes the setuid32() parameter
    **  uid=0.  According to the man page, this causes no action to be
    **  taken while the syscall returns with a successful result.
    **  
    **  In the erroneous case, this function:
    **   1) Sets the euid to the test user
    **   2) Discovers a uid that will result in a failure when passed
    **      to setuid32() by the test user
    **   3) Sets the euid to the superuser
    **   4) Clears the audit trail
    **   5) Sets the euid to the test user
    **   6) Attempts to set the uid to the unique, invalid uid
    **      determined in step (2)
    **   7) Sets the euid to the superuser
    **   8) Verifies that setuid32 call executed erroneously.
    **
    **  The erroneous case satisfies the condition for failure as
    **  detailed in the man page.  It attempts to set uid to a unique,
    **  invalid value which will cause setuid32() to return a failure
    **  code.
    **
    **  HISTORY    :
    **    06/03 Originated by Michael A. Halcrow <mike@halcrow.us>
    **    03/04 Added exp_errno variable by D. Kent Soper <dksoper@us.ibm.com>
    **    05/04 Updates to suppress compile warnings by Kimberlt D. Simon <kdsimon@us.ibm.com>
    **
    **********************************************************************/

#include "includes.h"
#include "syscalls.h"

int test_setuid32(struct audit_data *context)
{
    int rc = 0;
#if !defined(__PPC) && !defined(__X86_64) && !defined(__S390X) && !defined(__IA64)
    int exp_errno = EPERM;
    int uid;

     /**
      * Do as much setup work as possible right here
      */
    if (context->success) {
	uid = 0;
	context->euid = 0;
	context->egid = 0;
	context->fsuid = 0;
	context->fsgid = 0;
	fprintf(stderr, "Target uid=%d in success case\n", uid);
    } else {
	identifiers_t identifiers;
       /**
        * To test the failure case, the following conditions must apply:
        *  - I am not the superuser
        *  - The new uid CANNOT match any one of the following:
        *   - effective group ID
        *   - saved set-group-ID
        */
	// Pick a nice round ID, test it, and increment it on every
	// sequential failure until we find something that works
	uid = 42;

	// su to test user
	fprintf(stderr, "seteuid to %i\n", context->euid);
	if ((rc = seteuid(context->euid)) != 0) {
	    fprintf(stderr, "Unable to seteuid to %i: errno=%i\n",
		    context->euid, errno);
	    goto EXIT;		// Or possibly EXIT_CLEANUP
	}

	if ((rc = getIdentifiers(&identifiers) != 0)) {
	    fprintf(stderr, "Utility getIdentifiers failed\n");
	    goto EXIT;
	}
	while (uid == identifiers.euid || uid == identifiers.suid) {
	    uid++;
	}

	// su to superuser
	fprintf(stderr, "seteuid to root\n");
	if ((rc = seteuid(0)) != 0) {
	    fprintf(stderr, "Unable to seteuid to root: errno=%i\n", errno);
	    goto EXIT_CLEANUP;	// Or possibly EXIT_CLEANUP
	}

    }

    // Set up audit argument buffer
    if ((rc = auditArg1(context, AUDIT_ARG_IMMEDIATE, sizeof(int), &uid)) != 0) {
	fprintf(stderr, "Error setting up audit argument buffer\n");
	goto EXIT;
    }
    // Do pre-system call work  
    if ((rc = preSysCall(context)) != 0) {
	fprintf(stderr, "ERROR: pre-syscall setup failed (%d)\n", rc);
	goto EXIT_CLEANUP;
    }

    context->u.syscall.exit = syscall(__NR_setuid32, uid);

    // Do post-system call work
    if ((rc = postSysCall(context, errno, -1, exp_errno)) != 0) {
	fprintf(stderr, "ERROR: post-syscall setup failed (%d)\n", rc);
	goto EXIT_CLEANUP;
    }

EXIT_CLEANUP:
     /**
      * Do cleanup work here
      */

EXIT:
    fprintf(stderr, "Returning from test\n");
#endif
    return rc;
}
