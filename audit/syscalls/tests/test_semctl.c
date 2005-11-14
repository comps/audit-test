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
 **  FILE       : test_semctl.c
 **
 **  PURPOSE    : To test the semctl library call auditing.
 **
 **  DESCRIPTION: The test_semctl() function builds into the laus_test
 **  framework to verify that the Linux Audit System accurately logs
 **  both successful and erroneous execution of the "lpc" system call.
 **
 **  In the successful case, this function:
 **   1) Allocates a new semaphore via semget()
 **   2) Uses semctl() to deallocate the newly allocated semaphore
 **   3) Tests the result of the call against the expected successful
 **      return.
 **  
 **  The successful case uses the semid returned by semget() in using
 **  semctl() to deallocate the memory.  If semctl() returns 0, we have
 **  a success, as specified in the man page.
 **
 **  In the erroneous case, this function:
 **
 **  Semphore operations are attempted as a non-root user, thus
 **  causing an EPERM errno.
 **
 **  HISTORY    :
 **    06/03 Originated by Michael Halcrow <mike@halcrow.us>
 **    06/03 Furthered by Dustin Kirkland (k1rkland@us.ibm.com)
 **    03/04 Added exp_errno variable by D. Kent Soper <dksoper@us.ibm.com>
 **    04/04 Updated test for 2.6 LAuS implementation by Kimberly D. Simon <kdsimon@us.ibm.com>
 **    05/04 Updates to suppress compile warnings by Kimberly D. Simon <kdsimon@us.ibm.com>
 **
 **********************************************************************/

#include "includes.h"
#include "syscalls.h"
#include <sys/ipc.h>
#if defined(__PPC64)
#include <asm-ppc64/ipc.h>
#elif !defined(__IA64)
#include <asm/ipc.h>
#endif
#include <sys/sem.h>

int test_semctl(struct audit_data *context)
{
    int rc = 0;
    int exp_errno = EPERM;
    int semctlrc = 0;
    int semid = 0;
    int mode;
    int semnum = 0;		// this is ignored by semctl when we remove
    static int cmd = IPC_RMID;
    char *buf = NULL;

    // Allocate shared memory space so that we can test deallocation via
    // semctl
    mode = S_IRWXU;
    if ((semid = semget(IPC_PRIVATE, 1, mode)) == -1) {
	printf1("ERROR: Unable to allocate new semaphore: errno=%i\n", errno);
	goto EXIT;
    }
    if (context->success) {
	context->euid = 0;
	context->egid = 0;
	context->fsuid = 0;
	context->fsgid = 0;
    }
    // Set up audit argument buffer
    if ((rc = auditArg4(context,
			AUDIT_ARG_IMMEDIATE, sizeof(int), &semid,
			AUDIT_ARG_IMMEDIATE, sizeof(int), &semnum,
			AUDIT_ARG_IMMEDIATE, sizeof(int), &cmd,
#if (defined(__X86_64) || defined(__IA64)) && !defined(__MODE_32)
			AUDIT_ARG_POINTER, 0, &buf
#else
			AUDIT_ARG_NULL, 0, NULL
#endif
	 )) != 0) {
	printf1("Error setting up audit argument buffer\n");
	goto EXIT_CLEANUP;
    }
    // Do pre-system call work
    if ((rc = preSysCall(context)) != 0) {
	printf1("ERROR: pre-syscall setup failed (%d)\n", rc);
	goto EXIT_CLEANUP;
    }
    // Execute system call
#if (defined(__X86_64) || defined(__IA64)) && !defined(__MODE_32)
    context->u.syscall.exit = semctlrc =
	syscall(__NR_semctl, semid, 0, cmd, &buf);
#else
    context->u.syscall.exit = semctlrc =
	syscall(__NR_ipc, SEMCTL, semid, 0, cmd, &buf);
#endif
    // Do post-system call work
    if ((rc = postSysCall(context, errno, -1, exp_errno)) != 0) {
	printf1("ERROR: post-syscall setup failed (%d)\n", rc);
	goto EXIT_CLEANUP;
    }

EXIT_CLEANUP:

    if (!context->success && semid && (semid != -1)) {
	if ((rc = semctl(semid, 0, IPC_RMID)) == -1) {
	    printf1
		("Error removing semaphore set with ID = [%d]: errno = [%i]\n",
		 semid, errno);
	    goto EXIT;
	}
    }

EXIT:
    return rc;
}
