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
 **  FILE       : test_shmat.c
 **
 **  PURPOSE    : To test the shmat library call auditing.
 **
 **  DESCRIPTION: The test_shmat() function builds into the laus_test
 **  framework to verify that the Linux Audit System accurately logs
 **  both successful and erroneous execution of the "lpc" system call.
 **
 **  In the successful case, this function:
 **   1) Allocates a new block of shared memory
 **   2) Clears the audit trail
 **   3) Attaches to that block of shared memory
 **   4) Tests the result of the shmat() call against the expected
 **      successful return
 **   5) Detaches from the block of shared memory
 **   6) Deallocates the block of shared memory.
 **  
 **  The successful case attaches to the shared memory block specified
 **  by the shmid value returned by shmget().
 **
 **  In the erroneous case, this function:
 **
 **  In the erroneous case, we attempt to attach to shared memory
 **  as a non-root user, which causes an EACCES errno.
 **
 **  HISTORY    :
 **    06/03 Originated by Michael A. Halcrow <mike@halcrow.us>
 **    06/03 Furthered by Dustin Kirkland  (k1rkland@us.ibm.com)
 **    10/03 Extended to invoke EACCES by Michael A. Halcrow
 **          <mike@halcrow.us>
 **    03/04 Added exp_errno variable by D. Kent Soper <dksoper@us.ibm.com>
 **    05/04 Cleaned up usage of shmat by Kimberly D. Simon <kdsimon@us.ibm.com>
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


int test_shmat(laus_data *dataPtr)
{

    int rc = 0;
    int exp_errno = EACCES;
    int shmid = 0;
    static void *voidPtr = NULL;
    int shmflg = 0;
    void *shmptr = NULL;
    int mode;


    // Set the syscall-specific data
    printf5("Setting laus_var_data.syscallData.code to %d\n", AUDIT_shmat);
    dataPtr->laus_var_data.syscallData.code = AUDIT_shmat;

  /**
   * Do as much setup work as possible right here
   */
    if (dataPtr->successCase) {
	dataPtr->msg_euid = 0;
	dataPtr->msg_egid = 0;
	dataPtr->msg_fsuid = 0;
	dataPtr->msg_fsgid = 0;
	mode = S_IRWXU | S_IRWXG | S_IRWXO;
    } else {
	mode = S_IRWXU;
    }

    // Create the shared memory segment
    if ((shmid = shmget(IPC_PRIVATE, PAGE_SIZE, mode)) == -1) {
	printf1("ERROR: Unable to create shared memory segment\n");
	goto EXIT;
    }
    printf4("Generated shmid = [%d]\n", shmid);


    // Do pre-system call work
    if ((rc = preSysCall(dataPtr)) != 0) {
	printf1("ERROR: pre-syscall setup failed (%d)\n", rc);
	goto EXIT_CLEANUP;
    }
    // Execute system call
    shmptr = shmat(shmid, NULL, 0);
    if (shmptr != (void *)-1) {
	dataPtr->laus_var_data.syscallData.result = 0;
    } else {
	dataPtr->laus_var_data.syscallData.result = -1;
	dataPtr->laus_var_data.syscallData.resultErrno = errno;
    }

    // Strange location because kernel is filling in a value in raddr we need
    // Set up audit argument buffer
    if ((rc = auditArg3(dataPtr,
			AUDIT_ARG_IMMEDIATE, sizeof(int), &shmid,
			AUDIT_ARG_NULL, 0, voidPtr,
			AUDIT_ARG_IMMEDIATE, sizeof(int), &shmflg)) != 0) {
	printf1("Error setting up audit argument buffer\n");
	goto EXIT_CLEANUP;
    }
    // Do post-system call work
    if ((rc = postSysCall(dataPtr, errno, -1, exp_errno)) != 0) {
	printf1("ERROR: post-syscall setup failed (%d)\n", rc);
	goto EXIT_CLEANUP;
    }


EXIT_CLEANUP:
    // shared memory cleanup
    if (dataPtr->successCase) {
	// detach memory if successfully attached
	if (shmptr != (void *)-1) {
	    if (shmdt(shmptr) == -1) {
		printf1
		    ("ERROR: Unable to detach memory with shmid %d: errno=%i\n",
		     shmid, errno);
	    }
	}
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
