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
 **  FILE   : test_swapoff.c
 **
 **  PURPOSE: The test_swapoff() function builds into the laus_test
 **           framework to verify that the Linux Audit System accurately
 **           logs both successful and erroneous execution of the
 **           "swapoff" system call.
 **
 **           In the successful case, this function:
 **             1) Generates a unique file name
 **             2) Creates the temporary file, and runs mkswap()
 **             3) Executes swapon on the temporary file
 **             3) Executes the "swapoff" system call.
 **
 **           The successful case executes the expected conditions
 **           described by the "swapoff" system call manpage.  That is,
 **           the swapoff() function is called using a valid filename 
 **           that has been created with mkswap() and has been passed
 **           to swapon(), and then turns swapping off.
 **
 **            In the erroneous case, this function:
 **             1) Calls swapoff as a non-root test user
 **
 **            The erroneous case executes the faulty conditions
 **            described by the "EPERM" error under the "swapoff" system
 **            system call manpage.
 **
 **
 **  HISTORY:
 **    06/03 swapon originated by Dustin Kirkland
 **          (k1rkland@us.ibm.com)
 **    07/03 swapoff adapted from swapon by Michael A. Halcrow
 **          <mike@halcrow.us>
 **    03/04 Added exp_errno variable by D. Kent Soper <dksoper@us.ibm.com>
 **    05/04 Updates to suppress compile warnings by Kimberly D. Simon <kdsimon@us.ibm.com> 
 **
 **********************************************************************/

#include "includes.h"
#include "syscalls.h"
#include <asm/page.h>
#include <sys/swap.h>

/*
** execute a swapoff operation
*/
int test_swapoff(struct audit_data *context)
{
    int rc = 0;
    int exp_errno = EPERM;
    //int length;            // not needed?
    char *fileName = NULL;
    int swapflags = SWAP_FLAG_PREFER;

    char *cmd;

    // Set the syscall specific data
    context->u.syscall.sysnum = AUDIT_swapoff;
    // BUGBUG: Need to understand how to set up syscall parameters

    // dynamically create temp file name
    if ((rc = createTempFile(&fileName, S_IRWXU | S_IRWXG | S_IRWXO,
			     context->euid, context->egid)) == -1) {
	printf1("ERROR: Cannot create file %s\n", fileName);
	goto EXIT;
    }
    // swapoff setup

    // TODO: Do we want to keep this dependency on external
    // executables?  If so, we need to assert prerequisites about the
    // path settings of the calling environment (and/or check them
    // explicitely).

    cmd =
	(char *)
	malloc(strlen("dd if=/dev/zero of= bs=1024 count=1024 > /dev/null 2>&1")
	       + strlen(fileName) + 1);
    sprintf(cmd, "dd if=/dev/zero of=%s bs=1024 count=1024 > /dev/null 2>&1", fileName);	// BUG: This needs to be in the PATH, yet this is not verified.
    if (system(cmd)) {
	printf1("Could not create file %s\n", fileName);
	goto EXIT_UNLINK;
    }
    free(cmd);

    cmd = (char *)malloc(strlen("mkswap > /dev/null 2>&1") + strlen(fileName) + 1);	// BUG: This needs to be in the PATH, yet this is not verified.
    sprintf(cmd, "mkswap %s > /dev/null 2>&1", fileName);
    if (system(cmd) != 0) {
	printf1("Could not mkswap %s\n", fileName);
	goto EXIT_UNLINK;
    }
    free(cmd);

    if ((rc = swapon(fileName, swapflags)) == -1) {
	printf1("Error calling swapon( %s, 0x%x ); errno=%i\n", fileName,
		swapflags, errno);
	goto EXIT_UNLINK;
    }
    // if testing success case, create the file, and mkswap
    if (context->success) {
	// must be root to swapoff()
	context->euid = 0;
	context->egid = 0;
	context->fsuid = 0;
	context->fsgid = 0;
    }
    // Set up audit argument buffer
    if ((rc = auditArg1(context,
			context->success ? AUDIT_ARG_PATH : AUDIT_ARG_NULL,
			context->success ? strlen(fileName) : 0,
			fileName)) != 0) {
	printf1("Error setting up audit argument buffer\n");
	goto EXIT_SWAPOFF;
    }
    // Do pre-system call work
    if ((rc = preSysCall(context)) != 0) {
	printf1("ERROR: pre-syscall setup failed (%d)\n", rc);
	goto EXIT_SWAPOFF;
    }
    // Execute system call
    context->u.syscall.exit = syscall(__NR_swapoff, fileName);

    // Do post-system call work
    if ((rc = postSysCall(context, errno, -1, exp_errno)) != 0) {
	printf1("ERROR: post-syscall setup failed (%d)\n", rc);
    }

EXIT_SWAPOFF:
    if (!context->success) {
	if ((rc = swapoff(fileName)) != 0) {
	    printf1("ERROR: Unable to swapoff file %s: errno=%i\n",
		    fileName, errno);
	}
    }

EXIT_UNLINK:
    if ((unlink(fileName)) != 0) {
	printf1("ERROR: Unable to remove file %s: errno=%i\n", fileName, errno);
    }

EXIT:
    if (fileName)
	free(fileName);
    // Sleep to solve syncronization issues
//  sleep( 2 );
    return rc;
}
