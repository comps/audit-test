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
 **  FILE       : test_setsid.c
 **
 **  PURPOSE    : To test the setsid library call auditing.
 **
 **  DESCRIPTION: The test_setsid() function builds into the
 **  laus_test framework to verify that the Linux Audit System
 **  accurately logs both successful and erroneous execution of the
 **  "setsid" system call.
 **
 **  In the successful case, this function:
 **   1) Calls setsid
 **   2) Verifies a successful result.
 **
 **  The successful case makes the calling test process a process
 **  group leader for a new session.
 **  
 **  In the erroneous case, this function:
 **   1) Calls setsid
 **   2) Calls setsid again
 **   3) Verifies an error result.
 **      
 **  The erroneous case first makes the calling test process a process
 **  group leader for a new session.  It then attempts to create a new
 **  session as a process group leader.  According to the man page for
 **  setsid, setsid fails if the calling process is already a process
 **  group leader.
 **
 **  HISTORY    :
 **    06/03 Originated by Michael A. Halcrow <mike@halcrow.us>
 **    03/04 Added exp_errno variable by D. Kent Soper <dksoper@us.ibm.com>
 **    05/04 Updates to suppress compile warnings by Kimberly D. Simon <kdsimon@us.ibm.com>
 **
 **********************************************************************/

#include "includes.h"
#include "syscalls.h"
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/wait.h>

typedef struct errnoAndReturnValue_s {
    int returnValue;
    int savedErrno;
} errnoAndReturnValue_t;

int test_setsid(laus_data *dataPtr)
{

    int rc = 0;
    int exp_errno = EPERM;
    int pid;
    //key_t shmkey;      // variables not needed?
    //int shmsize;
    //int shmflg;
    int shmid;

    errnoAndReturnValue_t *earv;
    int savedErrno;

    // Set the syscall-specific data
    printf5("Setting laus_var_data.syscallData.code to %d\n", AUDIT_setsid);
    dataPtr->laus_var_data.syscallData.code = AUDIT_setsid;

  /**
   * Do as much setup work as possible right here
   */
    // Initialize IPC
    // Shared memory
    if ((shmid = shmget(IPC_PRIVATE, sizeof(errnoAndReturnValue_t),
			IPC_CREAT)) == -1) {
	printf1("Error getting shared memory: errno=%i\n", errno);
	goto EXIT;		// TODO: Explicitely account for the fact that the semaphore has been created at this point
    }
    dataPtr->msg_euid = 0;
    dataPtr->msg_egid = 0;
    dataPtr->msg_fsuid = 0;
    dataPtr->msg_fsgid = 0;
    if (dataPtr->successCase) {
	// Set up for success
    } else {
	// Set up for error
    }

    // Set up audit argument buffer
    if ((rc = auditArg0(dataPtr)) != 0) {
	printf1("Error setting up audit argument buffer\n");
	goto EXIT;
    }
    // Do pre-system call work
    if ((rc = preSysCall(dataPtr)) != 0) {
	printf1("ERROR: pre-syscall setup failed (%d)\n", rc);
	goto EXIT_CLEANUP;
    }
    // Execute system call
    if ((pid = fork()) == 0) {
	errnoAndReturnValue_t *childEarv;
	if (!dataPtr->successCase) {
	    setsid();
	}
	if (((long)(childEarv = shmat(shmid, NULL, 0))) == -1) {
	    printf1
		("Error attaching to shared memory segment with id %d: errno=%i\n",
		 shmid, errno);
	    // TODO: Something a bit more drastic should happen at this point
	    _exit(0);
	}
	childEarv->returnValue = syscall(__NR_setsid);
	childEarv->savedErrno = errno;
	if (shmdt(childEarv) == -1) {
	    printf1
		("Error detaching from shared memory segment at address 0x%p: errno=%i\n",
		 childEarv, errno);
	    _exit(0);
	}
	_exit(0);
    } else {
	dataPtr->msg_pid = pid;
	if (waitpid(pid, NULL, 0) == -1) {
	    printf1("Error waiting on pid %d: errno=%i\n", pid, errno);
	    goto EXIT_CLEANUP;
	}
	if (((long)(earv = shmat(shmid, NULL, 0))) == -1) {
	    printf1
		("Error attaching to shared memory segment with id %d: errno=%i\n",
		 shmid, errno);
	    goto EXIT_CLEANUP;
	}
	dataPtr->laus_var_data.syscallData.result = earv->returnValue;
	savedErrno = earv->savedErrno;
	if (shmdt(earv) == -1) {
	    printf1
		("Error detaching from shared memory segment at address 0x%p: errno=%i\n",
		 earv, errno);
	    goto EXIT_CLEANUP;
	}
    }

    // Do post-system call work
    if ((rc = postSysCall(dataPtr, savedErrno, -1, exp_errno)) != 0) {
	printf1("ERROR: post-syscall setup failed (%d)\n", rc);
	goto EXIT_CLEANUP;
    }

EXIT_CLEANUP:
  /**
   * Do cleanup work here
   */
    // Release the shared memory
    if (shmctl(shmid, 0, IPC_RMID) == -1) {
	printf1("Error removing shared memory with id %d: errno=%i\n", shmid,
		errno);
	goto EXIT;
    }
    // Release the shared memory
    if (dataPtr->successCase) {
	// Clean up from success case setup
    }

EXIT:
    printf5("Returning from test\n");
    return rc;
}
