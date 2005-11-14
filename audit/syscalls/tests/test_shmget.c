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
 **  FILE       : test_shmget.c
 **
 **  PURPOSE    : To test the shmget library call auditing.
 **
 **  DESCRIPTION: The test_shmget() function builds into the laus_test
 **  framework to verify that the Linux Audit System accurately logs
 **  both successful and erroneous execution of the "lpc" system call.
 **
 **  In the successful case, this function:
 **   1) Sets the shared memory mode flags
 **   2) Clears the audit trail
 **   3) Sets key to IPC_PRIVATE
 **   4) Executes the "shmget" library call
 **   5) Tests the result of the call against the expected successful
 **      return
 **   6) Deallocates the newly allocated block of shared memory.
 **  
 **  The successful case executes the expected conditions
 **  described by the "shmget" library call man page.  That is,
 **  the shmget() function is called using IPC_PRIVATE for the key
 **  value.  "size" is set to PAGE_SIZE, and "mode" is set to ( S_IRWXU
 **  | S_IRWXG | S_IRWXO ).  The function returns an integer value
 **  specifying the shared memory ID for the newly allocated block of
 **  memory, which should be a valid value (not NULL or -1).
 **
 **  In the erroneous case, we attempt to access shared memory
 **  as a non-root user, which causes an EACCES errno.
 **
 **  HISTORY    :
 **    06/03 Originated by Michael A. Halcrow <mike@halcrow.us>
 **    06/03 Furthered by Dustin Kirkland (k1rkland@us.ibm.com)
 **    10/03 Extended to invoke EACCES errno by Michael A. Halcrow
 **          <mike@halcrow.us>
 **    03/04 Added exp_errno variable by D. Kent Soper <dksoper@us.ibm.com>
 **    05/04 Updates to suppress compile warnings by Kimberlt D. Simon <kdsimon@us.ibm.com>
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
#include <asm/page.h>
#include <sys/shm.h>

int test_shmget(struct audit_data *context)
{
    int rc = 0;
    int exp_errno = EACCES;
    int key = 0;
    int firstShmid = 0;
    int secondShmid = 0;
    //int doNotDeallocate = 0;
    int mode;
    static int pageSize;

    // Set the mode flags
    mode = S_IRWXU | S_IRWXG | S_IRWXO;

    // Set the key value.
    // If successCase == 0, then we will be double-allocating the memory
    // to force an error condition. 
    if (context->success) {
	key = IPC_PRIVATE;
    } else {
	mode = 0600 | IPC_CREAT;
	key = -1;
	if ((firstShmid = shmget(key, PAGE_SIZE, mode)) == -1) {
	    printf1
		("Cannot create the shared memory segment with key = -1: errno = [%i]\n",
		 errno);
	    goto EXIT;
	}
    }

    // Set up audit argument buffer
    pageSize = PAGE_SIZE;	/* macro on IA64 */
    if ((rc = auditArg3(context,
			AUDIT_ARG_IMMEDIATE, sizeof(int), &key,
			AUDIT_ARG_IMMEDIATE, sizeof(int), &pageSize,
			AUDIT_ARG_IMMEDIATE, sizeof(int), &mode)) != 0) {
	printf1("Error setting up audit argument buffer\n");
	goto EXIT_CLEANUP;
    }
    // Do pre-system call work
    if ((rc = preSysCall(context)) != 0) {
	printf1("ERROR: pre-syscall setup failed (%d)\n", rc);
	goto EXIT_CLEANUP;
    }
    // Execute system call
    //  context->u.syscall.exit = secondShmid = shmget( key, PAGE_SIZE, mode );
#if (defined(__X86_64) || defined(__IA64)) && !defined(__MODE_32)
    context->u.syscall.exit = secondShmid =
	syscall(__NR_shmget, key, PAGE_SIZE, mode);
#else
    context->u.syscall.exit = secondShmid =
	syscall(__NR_ipc, SHMGET, key, PAGE_SIZE, mode);
#endif
    // Do post-system call work
    if ((rc = postSysCall(context, errno, -1, exp_errno)) != 0) {
	printf1("ERROR: post-syscall setup failed (%d)\n", rc);
	goto EXIT_CLEANUP;
    }

EXIT_CLEANUP:
    if (firstShmid && (firstShmid != -1)) {
	if ((shmctl(firstShmid, IPC_RMID, 0)) == -1) {
	    printf1
		("ERROR: Cannot deallocate shared memory with shmid=%d: errno=%i\n",
		 firstShmid, errno);
	}
    }
    if (secondShmid && (secondShmid != -1)) {
	if ((shmctl(secondShmid, IPC_RMID, 0)) == -1) {
	    printf1
		("ERROR: Cannot deallocate shared memory with shmid=%d: errno=%i\n",
		 secondShmid, errno);
	    goto EXIT;
	}
    }

EXIT:
    return rc;
}
