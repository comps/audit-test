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
 **  FILE       : test_setfsgid32.c
 **
 **  PURPOSE    : To test the setfsgid32 library call auditing.
 **
 **  DESCRIPTION: The test_setfsgid32() function builds into the
 **  laus_test framework to verify that the Linux Audit System
 **  accurately logs both successful and erroneous execution of the
 **  "setfsgid32" system call.
 **
 **  In the successful case, this function:
 **   1) Sets the fsgid to the test user's gid
 **   2) Verifies that the fsgid was successfully set to the test
 **      user's gid
 **   3) Sets the fsgid to superuser.
 **
 **  The successful case executes setfsgid32() as the root user,
 **  guaranteeing success regardless of the parameter passed to
 **  setfsgid32(), in accordance with the description found in the man
 **  page for setfsgid32().
 **  
 **  In the erroneous case, this function:
 **   1) Sets the euid to the test user
 **   2) Discovers an fsgid that will result in a failure when passed
 **      to setfsgid32() by the test user
 **   3) Sets the euid to the superuser
 **   4) Sets the euid to the test user
 **   5) Attempts to set the fsgid to the unique, invalid fsgid
 **      determined in step (2)
 **   6) Sets the euid to the superuser
 **   7) Verifies that the fsgid was not set to the unique, invalid
 **      fsgid.
 **      
 **  The erroneous case satisfies the two conditions for failure
 **  specified in the man page for setfsgid32.  The process does not
 **  have ruid superuser, and the fsgid does not match the ruid, the
 **  euid, the fsuid, or the suid.  Thus, when the test user executes
 **  setfsgid32, we have the erroneous case.
 **
 **  HISTORY    :
 **    06/03 Originated by Michael A. Halcrow <mike@halcrow.us>
 **    03/04 Added exp_errno variable by D. Kent Soper <dksoper@us.ibm.com>
 **    05/04 Updates to suppress compile warnings by Kimberly D. Simon <kdsimon@us.ibm.com>
 **
 **********************************************************************/

#if !defined(__PPC) && !defined(__X86_64) && !defined(__S390X) && !defined(__IA64)

#include "includes.h"
#include "syscalls.h"

int test_setfsgid32(struct audit_data *context)
{
    int rc = 0;
    int exp_errno = EPERM;

    int fsgid;
    //int resultFsgid;    // variables not needed?
    //int originalEgid = context->egid;  

    identifiers_t identifiers;

	/**
	 * Do as much setup work as possible right here
	 */
    setfsgid(0);
    if (context->success) {
	fsgid = context->egid;
	context->euid = 0;
    } else {
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
	fsgid = 42;

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
	while (fsgid == identifiers.rgid || fsgid == identifiers.egid ||
	       fsgid == identifiers.sgid || fsgid == identifiers.fsgid) {
	    fsgid++;
	}

	// su to superuser
	printf5("seteuid to root\n");
	if ((rc = seteuid(0)) != 0) {
	    printf1("Unable to seteuid to root: errno=%i\n", errno);
	    goto EXIT_CLEANUP;	// Or possibly EXIT_CLEANUP
	}
    }

    // Set up audit argument buffer
    if ((rc = auditArg1(context,
			AUDIT_ARG_IMMEDIATE, sizeof(int), &fsgid)) != 0) {
	printf1("Error setting up audit argument buffer\n");
	goto EXIT;
    }

    // Do pre-system call work
    if ((rc = preSysCall(context)) != 0) {
	printf1("ERROR: pre-syscall setup failed (%d)\n", rc);
	goto EXIT_CLEANUP;
    }
    // Make system call
    context->u.syscall.exit = syscall(__NR_setfsgid32, fsgid);

    // Do post-system call work

    // Manpage for setfsgid says:
    // BUGS
    //    No error messages of any kind are returned to the  caller.
    //    At  the very least, EPERM should be returned when the call
    //    fails.
    // thus, we hardwire actual and expected errnos together
    errno = exp_errno;

    // Manpage also says:
    // RETURN VALUE
    //      On success, the previous value of fsgid is returned.  On error, the current value of fsgid is returned.
    // However, this is stupid, and LAuS recognizes this.
    // LAuS shows a return of 0 on success, and -1 on error (as any sane system call returns).  The test
    // has been modified to look for these adjusted return codes.

    if (!context->success) {
	context->u.syscall.exit = -1;
    } else {
	context->u.syscall.exit = 0;
	context->fsuid = 0;
    }

    if ((rc = postSysCall(context, errno, -1, exp_errno)) != 0) {
	printf1("ERROR: post-syscall setup failed (%d)\n", rc);
	goto EXIT_CLEANUP;
    }

EXIT_CLEANUP:
	/**
	 * Do cleanup work here
	 */
    if (context->success) {
	setfsgid(0);
    }

EXIT:
    printf5("Returning from test\n");
    return rc;
}

#endif
