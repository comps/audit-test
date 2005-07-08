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
   **  FILE       : test_shmdt.c
   **
   **  PURPOSE    : To test the shmdt library call auditing.
   **
   **  DESCRIPTION: The test_shmdt() function builds into the laus_test
   **  framework to verify that the Linux Audit System accurately logs
   **  both successful and erroneous execution of the "lpc" system call.
   **
   **  In the successful case, this function:
   **   1) Allocates a new block of shared memory
   **   2) Attaches to that block of shared memory
   **   3) Clears the audit trail
   **   4) Detaches from the block of shared memory
   **   5) Tests the result of the shmdt() call against the expected
   **      successful return
   **   6) Deallocates the block of shared memory.
   **  
   **  The successful case attempts to detach from a block of memory
   **  allocated and attached beforehand.  On a successful detach, we get
   **  a 0 result from shmdt().
   **
   **  In the erroneous case, this function:
   **   1) Clears the audit trail
   **   2) Attempts to detach from a NULL memory location
   **   3) Tests the results of the shmdt() call against the expected
   **      erroneous return
   **
   **  NULL is an invalid shmaddr value, and so, according to the man
   **  page for shmdt(), it should return a -1 and set errno to EINVAL
   **  when we test the erroneous case.
   **
   **  HISTORY    :
   **    06/03 Originated by Michael A. Halcrow <mike@halcrow.us>
   **    06/03 Furthered by Dustin Kirkland (k1rkland@us.ibm.com)
   **    03/04 Added exp_errno variable by D. Kent Soper <dksoper@us.ibm.com>
   **
   **********************************************************************/

#include "includes.h"
#include "syscalls.h"
#include <sys/shm.h>
#include <asm/page.h>
#if defined(__PPC64)
#include <asm-ppc64/ipc.h>
#elif !defined(__IA64)
#include <asm/ipc.h>
#endif

int test_shmdt(laus_data *dataPtr)
{

    int rc = 0;
    int exp_errno = EINVAL;
    int shmid = 0;
    void *shmptr = NULL;
    int dtrc = 0;
    int mode;

    // Set the syscall-specific data
    printf5("Setting laus_var_data.syscallData.code to %d\n", AUDIT_shmdt);
    dataPtr->laus_var_data.syscallData.code = AUDIT_shmdt;

     /**
      * Do as much setup work as possible right here
      */
    // Create the shared memory segment
    if (dataPtr->successCase) {
	mode = S_IRWXU | S_IRWXG | S_IRWXO;
	if ((shmid = shmget(IPC_PRIVATE, PAGE_SIZE, mode)) == -1) {
	    printf1("ERROR: Unable to create shared memory segment\n");
	    goto EXIT;
	}
    } else {
	shmid = -1;
    }

    // su to test user
    printf5("seteuid to %i\n", dataPtr->msg_euid);
    if ((rc = seteuid(dataPtr->msg_euid)) != 0) {
	printf1("ERROR: Unable to seteuid to %i: errno=%i\n",
		dataPtr->msg_euid, errno);
	goto EXIT_FREE_SHM;
    }
    // Attach to the shared memory
    if (dataPtr->successCase) {
	if ((long)(shmptr = shmat(shmid, NULL, 0)) == -1) {
	    printf1
		("ERROR: Unable to attach to shared memory with shmid %d: errno=%i\n",
		 shmid, errno);
	    goto EXIT_FREE_SHM;
	}
    }
    // su back to root
    printf5("seteuid to root\n");
    if ((rc = seteuid(0)) != 0) {
	printf1("ERROR: Unable to seteuid to root: errno=%i\n", errno);
	goto EXIT_FREE_SHM;
    }
    // Set up audit argument buffer
    if ((rc = auditArg1(dataPtr,
			(shmptr == NULL ? AUDIT_ARG_NULL : AUDIT_ARG_POINTER),
			0, shmptr)) != 0) {
	printf1("Error setting up audit argument buffer\n");
	goto EXIT;
    }
    // Do pre-system call work
    if ((rc = preSysCall(dataPtr)) != 0) {
	printf1("ERROR: pre-syscall setup failed (%d)\n", rc);
	goto EXIT_CLEANUP;
    }
    // Execute system call
    dataPtr->laus_var_data.syscallData.result = dtrc = shmdt(shmptr);

    // Do post-system call work
    if ((rc = postSysCall(dataPtr, errno, -1, exp_errno)) != 0) {
	printf1("ERROR: post-syscall setup failed (%d)\n", rc);
	goto EXIT_CLEANUP;
    }


EXIT_CLEANUP:

EXIT_FREE_SHM:
    // shared memory cleanup
    if (dataPtr->successCase) {	// Shared memory is only allocated in the success case
	if (shmctl(shmid, IPC_RMID, 0) == -1) {
	    printf1
		("ERROR: Unable to free shared memory with shmid %d: errno=%i\n",
		 shmid, errno);
	    goto EXIT;
	}
    }

EXIT:
    return rc;
}
