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
 **  FILE       : test_setfsuid32.c
 **
 **  PURPOSE    : To test the setfsuid32 library call auditing.
 **
 **  DESCRIPTION: The test_setfsuid32() function builds into the
 **  laus_test framework to verify that the Linux Audit System
 **  accurately logs both successful and erroneous execution of the
 **  "setfsuid32" system call.
 **
 **  In the successful case, this function:
 **   1) Becomes superuser
 **   2) Sets the fsuid to the test user's gid
 **   3) Verifies that the fsuid was successfully set to the test
 **      user's gid
 **
 **  The successful case executes setfsuid32() as the root user,
 **  guaranteeing success regardless of the parameter passed to
 **  setfsuid32(), in accordance with the description found in the man
 **  page for setfsuid32().
 **  
 **  In the erroneous case, this function:
 **   1) Sets the euid to the test user
 **   2) Discovers an fsuid that will result in a failure when passed
 **      to setfsuid32() by the test user
 **   3) Sets the euid to the superuser
 **   4) Sets the euid to the test user
 **   5) Attempts to set the fsuid to the unique, invalid fsuid
 **      determined in step (2)
 **   6) Attempts to set the fsuid to the unique, invalid fsuid
 **      determined in step (2) a second time
 **   7) Sets the euid to the superuser
 **   8) Verifies that the fsuid was not set to the unique, invalid
 **      fsuid on the first try.
 **      
 **  The erroneous case satisfies the two conditions for failure
 **  specified in the man page for setfsuid32.  The process does not
 **  have ruid superuser, and the fsuid does not match the ruid, the
 **  euid, the fsuid, or the suid.  Thus, when the test user executes
 **  setfsuid32, we have the erroneous case.
 **
 **  HISTORY    :
 **    06/03 Originated by Michael A. Halcrow <mike@halcrow.us>
 **    03/04 Added exp_errno variable by D. Kent Soper <dksoper@us.ibm.com>
 **    05/04 Updates to suppress compile warnings by Kimberly D. Simon <kdsimon@us.ibm.com>
 **
 **********************************************************************/

#include "includes.h"

int test_setfsuid32(struct audit_data *context)
{
    int rc = 0;
#if !defined(__PPC) && !defined(__X86_64) && !defined(__S390X) && !defined(__IA64)
    int exp_errno = EPERM;

    int secondFsuid;

    //int failureRc;    //not needed?

  /**
   * Do as much setup work as possible right here
   */
    if (context->success) {
	secondFsuid = context->euid;
	context->euid = 0;
    } else {
	int nonexistentFsuid;
	identifiers_t identifiers;
    /**
     * To test the failure case, the following conditions must apply:
     *  - I am not the superuser
     *  - The new fsuid CANNOT match any one of the following:
     *   - real group ID
     *   - effective group ID
     *   - saved set-group-ID
     *   - current value of fsuid
     */
	// Pick a nice round ID, test it, and increment it on every
	// sequential failure until we find something that works
	nonexistentFsuid = 42;

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

	while (nonexistentFsuid == identifiers.ruid ||
	       nonexistentFsuid == identifiers.euid ||
	       nonexistentFsuid == identifiers.suid ||
	       nonexistentFsuid == identifiers.fsuid) {
	    nonexistentFsuid++;
	}

	// su to superuser
	printf5("seteuid to root\n");
	if ((rc = seteuid(0)) != 0) {
	    printf1("Unable to seteuid to root: errno=%i\n", errno);
	    goto EXIT_CLEANUP;	// Or possibly EXIT_CLEANUP
	}

	secondFsuid = nonexistentFsuid;	// Both attempts will return msg_euid
    }

    // Set up audit argument buffer
    if ((rc = auditArg1(context,
			AUDIT_ARG_IMMEDIATE, sizeof(int), &secondFsuid)) != 0) {
	printf1("Error setting up audit argument buffer\n");
	goto EXIT;
    }
    // Do pre-system call work
    if ((rc = preSysCall(context)) != 0) {
	printf1("ERROR: pre-syscall setup failed (%d)\n", rc);
	goto EXIT_CLEANUP;
    }
    // Execute system call
    context->u.syscall.exit = syscall(__NR_setfsuid32, secondFsuid);

    // Do post-system call work
    if (!context->success) {
	context->u.syscall.exit = -1;
	errno = exp_errno;
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
	setfsuid(0);
    }

EXIT:
    printf5("Returning from test\n");
#endif
    return rc;
}
