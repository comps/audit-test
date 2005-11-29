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
 **  FILE   : test_utime.c
 **
 **  PURPOSE: The test_utime() function builds into the laus_test
 **           framework to verify that the Linux Audit System accurately
 **           logs both successful and erroneous execution of the
 **           "utime" system call.
 **
 **           In the successful case, this function:
 **             1) Creates the temporary file
 **             2) Creates the utime data structure
 **             3) Executes the "utime" system call
 **
 **           The successful case executes the expected conditions
 **           described by the "utime" system call manpage.  That is,
 **           the utime() function is called using a valid filename 
 **           and actually changes the access and modified timestamps
 **           on the file.
 **
 **            In the erroneous case, this function:
 **             1) Creates the temporary file
 **             2) Creates the utime data structure
 **             3) Executes the "utime" system call as helper_uid
 **
 **            The erroneous case executes the faulty conditions
 **            described by the "EPERM" errno.
 **            That is, the utime() function is called on NULL.
 **
 **
 **  HISTORY:
 **    06/03 originated by Dustin Kirkland (k1rkland@us.ibm.com)
 **    10/03 furthered by Kylene J. Smith (kylene@us.ibm.com)
 **    03/04 Added exp_errno variable by D. Kent Soper <dksoper@us.ibm.com>
 **
 **********************************************************************/

#include "includes.h"
#include "syscalls.h"
#include <utime.h>
#include <sys/time.h>

/*
 ** execute a utime operation
 */
int test_utime(struct audit_data *context, int variation, int success)
{
    int rc = 0;
#ifdef __NR_utime
    int exp_errno = -EPERM;
    char *fileName = NULL;
    struct timespec mod_time, acc_time;
    struct utimbuf utbuf;

    // Create the file 
    fileName = init_tempfile(S_IRWXU|S_IRWXG|S_IRWXO, context->euid,
			     context->egid);
    if (!fileName) {
	rc = -1;
	goto EXIT;
    }
    // utime setup
    utbuf.modtime = mod_time.tv_sec = 10;
    mod_time.tv_nsec = 0;
    utbuf.actime = acc_time.tv_sec = 30;
    acc_time.tv_nsec = 0;

    if (!context->success) {

	context->euid = context->fsuid = helper_uid;
    }

    // Set up audit argument buffer
    if ((rc = auditArg3(context,
			AUDIT_ARG_PATH, strlen(fileName), fileName,
			AUDIT_ARG_POINTER, sizeof(struct timespec), &acc_time, 
			AUDIT_ARG_POINTER, sizeof(struct timespec), &mod_time)
	 ) != 0) {
	fprintf(stderr, "Error setting up audit argument buffer\n");
	goto EXIT;
    }
    // Do pre-system call work
    if ((rc = preSysCall(context)) != 0) {
	fprintf(stderr, "ERROR: pre-syscall setup failed (%d)\n", rc);
	goto EXIT_CLEANUP;
    }
    // Execute system call
    context->u.syscall.exit = utime(fileName, &utbuf);

    // Do post-system call work
    if ((rc = postSysCall(context, errno, -1, exp_errno)) != 0) {
	fprintf(stderr, "ERROR: post-syscall setup failed (%d)\n", rc);
	goto EXIT_CLEANUP;
    }

EXIT_CLEANUP:
    // utime cleanup
    if ((rc = unlink(fileName)) != 0) {
	fprintf(stderr, "ERROR: Unable to remove file %s: errno=%i\n", fileName, errno);
	goto EXIT;
    }

EXIT:
    if (fileName)
	free(fileName);
#endif /* __NR_utime */
    return rc;
}
