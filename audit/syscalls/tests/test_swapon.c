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
   **  FILE   : test_swapon.c
   **
   **  PURPOSE: The test_swapon() function builds into the laus_test
   **           framework to verify that the Linux Audit System accurately
   **           logs both successful and erroneous execution of the
   **           "swapon" system call.
   **
   **           In the successful case, this function:
   **             1) Generates a unique file name
   **             2) Creates the temporary file, and runs mkswap()
   **             3) Executes the "swapon" system call
   **
   **           The successful case executes the expected conditions
   **           described by the "swapon" system call manpage.  That is,
   **           the swapon() function is called using a valid filename 
   **           that has been created with mkswap(), and then turns 
   **           swapping on.
   **
   **            In the erroneous case, this function:
   **             1) Calls swapon with as a non-root test user
   **
   **            The erroneous case executes the faulty conditions
   **            described by the "EPERM" error under the "swapon" system
   **            system call manpage.  
   **
   **
   **  HISTORY:
   **    06/03 originated by Dustin Kirkland (k1rkland@us.ibm.com)
   **    03/04 Added exp_errno variable by D. Kent Soper <dksoper@us.ibm.com>
   **    05/04 Updates to suppress warnings by Kimberly D. Simon <kdsimon@us.ibm.com>
   **
   **********************************************************************/

#include "includes.h"
#include "syscalls.h"
#include <asm/page.h>
#include <sys/swap.h>

   /*
    ** execute a swapon operation
    */
int test_swapon(struct audit_data *context, int variation, int success)
{
    int rc = 0;
    int exp_errno = -EPERM;
    //int length;           // variables not needed?
    //size_t count = 80;
    char *fileName = NULL;
    int swapflags = SWAP_FLAG_PREFER;

    char *cmd;

    // dynamically create temp file name
    fileName = init_tempfile(S_IRWXU|S_IRWXG|S_IRWXO, context->euid,
			     context->egid);
    if (!fileName) {
	rc = -1;
	goto EXIT;
    }
    // swapon setup

    // TODO: Do we want to keep this dependency on external
    // executables?  If so, we need to assert prerequisites about the
    // path settings of the calling environment (and/or check them
    // explicitely).
    cmd =
	(char *)
	malloc(strlen("dd if=/dev/zero of= bs=1024 count=1024 > /dev/null 2>&1")
	       + strlen(fileName) + 1);
    sprintf(cmd, "dd if=/dev/zero of=%s bs=1024 count=1024 > /dev/null 2>&1",
	    fileName);
    if (system(cmd)) {
	fprintf(stderr, "Could not create file %s\n", fileName);
	goto EXIT;
    }
    free(cmd);

    cmd =
	(char *)malloc(strlen("mkswap > /dev/null 2>&1") + strlen(fileName) +
		       1);
    sprintf(cmd, "mkswap %s > /dev/null 2>&1", fileName);
    if (system(cmd) != 0) {
	fprintf(stderr, "Could not mkswap %s\n", fileName);
	goto EXIT_CLEANUP;
    }
    free(cmd);

    // if testing success case, create the file, and mkswap
    if (context->success) {
	// must be root to swapon()
	context->euid = 0;
	context->egid = 0;
	context->fsuid = 0;
	context->fsgid = 0;
    }
    // Set up audit argument buffer
    if ((rc = auditArg2(context,
			context->success ? AUDIT_ARG_PATH : AUDIT_ARG_NULL,
			context->success ? strlen(fileName) : 0, fileName,
			AUDIT_ARG_IMMEDIATE, sizeof(swapflags),
			&swapflags)) != 0) {
	fprintf(stderr, "Error setting up audit argument buffer\n");
	goto EXIT;
    }
    // Do pre-system call work
    if ((rc = preSysCall(context)) != 0) {
	fprintf(stderr, "ERROR: pre-syscall setup failed (%d)\n", rc);
	goto EXIT_CLEANUP;
    }
    // Execute system call
    context->u.syscall.exit = swapon(fileName, swapflags);

    // Do post-system call work
    if ((rc = postSysCall(context, errno, -1, exp_errno)) != 0) {
	fprintf(stderr, "ERROR: post-syscall setup failed (%d)\n", rc);
	goto EXIT_CLEANUP;
    }

EXIT_CLEANUP:
    // swapon cleanup
    if (context->success) {
	if ((swapoff(fileName)) != 0) {
	    fprintf(stderr, "ERROR: Unable to swapoff file %s: errno=%i\n",
		    fileName, errno);
	}
    }
    if ((unlink(fileName)) != 0) {
	fprintf(stderr, "ERROR: Unable to remove file %s: errno=%i\n", fileName, errno);
    }

EXIT:
    if (fileName)
	free(fileName);
    return rc;
}
