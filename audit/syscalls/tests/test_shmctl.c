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
 **  FILE       : test_shmctl.c
 **
 **  PURPOSE    : To test the shmctl library call auditing.
 **
 **  DESCRIPTION: The test_shmctl() function builds into the laus_test
 **  framework to verify that the Linux Audit System accurately logs
 **  both successful and erroneous execution of the "lpc" system call.
 **
 **  In the successful case, this function:
 **   1) Clears the audit trail
 **   2) Allocates a new block of shared memory via shmget()
 **   3) Uses shmctl() to deallocate the newly allocated block of memory
 **   4) Tests the result of the call against the expected successful
 **      return.
 **  
 **  The successful case uses the shmid returned by shmget() in using
 **  shmctl() to deallocate the memory.  If shmctl() returns 0, we have
 **  a success, as specified in the man page.
 **
 **  In the erroneous case, we attempt to allocate shared memory
 **  as a non-root user, which causes an EPERM errno.
 **
 **  HISTORY    :
 **    06/03 Originated by Michael A. Halcrow <mike@halcrow.us>
 **    06/03 Furthered by Dustin Kirkland (k1rkland@us.ibm.com)
 **    10/03 Extended by Michael A. Halcrow <mike@halcrow.us> to
 **          invoke EPERM errno
 **    03/04 Added exp_errno variable by D. Kent Soper <dksoper@us.ibm.com>
 **    04/04 Updated test for 2.6 LAuS implementation by Kimberly D. Simon <kdsimon@us.ibm.com>
 **    05/04 Updates to suppress compile warnings by Kimberlt D. Simon <kdsimon@us.ibm.com>
 **    10/04 Added special case for s390x in 31 bit mode by Loulwa Salem <loulwa@us.ibm.com>
 **
 **********************************************************************/

#include "includes.h"
#include "syscalls.h"
#include <sys/ipc.h>
#if defined(__powerpc64__)
#include <asm-ppc64/ipc.h>
#elif !defined(__ia64__)
#include <asm/ipc.h>
#endif
#include <sys/shm.h>
#include <asm/page.h>

int test_shmctl(struct audit_data *context)
{
    int rc = 0;
    int exp_errno = -EPERM;
    int shmctlrc = 0;
    int shmid = 0;
    //int third_arg = 0;
    //key_t key;
    int mode;
    static int cmd;
    struct shmid_ds buf;

  /**
   * Do as much setup work as possible right here
   */

    memset(&buf, 0, sizeof(buf));

    mode = S_IRWXU;
    // Allocate shared memory space so that we can test deallocation via shmct
    if ((shmid = shmget(IPC_PRIVATE, PAGE_SIZE, mode)) == -1) {
	fprintf
	    (stderr,
	     "ERROR: Unable to allocate new shared memory segment: errno=%i\n",
	     errno);
	goto EXIT;
    }

    if (context->success) {
	context->euid = 0;
	context->egid = 0;
	context->fsuid = 0;
	context->fsgid = 0;
    } else {
    }

    // Set up audit argument buffer

// Handle special case on zSeries in 31 bit mode.
// IPC_RMID needs to be ored with IPC_64 flag to get the correct value
#if defined(__s390x__) // && defined(__MODE_32)
#error This #if needs to be updated to test something other than __MODE_32
    cmd = IPC_RMID | 0x0100;	// Value of IPC_64 as defined in /usr/include/linux/ipc.h
#else
    cmd = IPC_RMID;
#endif

#if (defined(__x86_64__) || defined(__ia64__))
    if ((rc = auditArg3(context,
			AUDIT_ARG_IMMEDIATE, sizeof(int), &shmid,
			AUDIT_ARG_IMMEDIATE, sizeof(int), &cmd,
			AUDIT_ARG_NULL, 0, NULL)) != 0) {
#else
    if ((rc = auditArg3(context,
			AUDIT_ARG_IMMEDIATE, sizeof(int), &shmid,
			AUDIT_ARG_IMMEDIATE, sizeof(int), &cmd,
			AUDIT_ARG_POINTER, 0, &buf)) != 0) {
#endif
	fprintf(stderr, "Error setting up audit argument buffer\n");
	goto EXIT;
    }
    // Do pre-system call work
    if ((rc = preSysCall(context)) != 0) {
	fprintf(stderr, "ERROR: pre-syscall setup failed (%d)\n", rc);
	goto EXIT_CLEANUP;
    }
    // Execute system call
#if (defined(__x86_64__) || defined(__ia64__))
    context->u.syscall.exit = shmctlrc = syscall(__NR_shmctl, shmid,
						 IPC_RMID, 0, &buf);
#else
    context->u.syscall.exit = shmctlrc =
	syscall(__NR_ipc, SHMCTL, shmid, IPC_RMID, 0, &buf);
#endif

    // Do post-system call work
    if ((rc = postSysCall(context, errno, -1, exp_errno)) != 0) {
	fprintf(stderr, "ERROR: post-syscall setup failed (%d)\n", rc);
	goto EXIT_CLEANUP;
    }

EXIT_CLEANUP:
    if (!context->success && shmid && (shmid != -1)) {
	if ((rc = shmctl(shmid, IPC_RMID, 0)) == -1) {
	    fprintf
		(stderr,
		 "Error removind shared memory with ID = [%d]: errno = [%i]\n",
		 shmid, errno);
	    goto EXIT;
	}
    }

EXIT:
    return rc;
}
