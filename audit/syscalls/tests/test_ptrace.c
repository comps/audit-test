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
 **  FILE       : test_ptrace.c
 **
 **  PURPOSE    : To test the ptrace library call auditing.
 **
 **  DESCRIPTION: The test_ptrace() function builds into the
 **  laus_test framework to verify that the Linux Audit System
 **  accurately logs both successful and erroneous execution of the
 **  "ptrace" system call.
 **
 **  In the successful case, this function:
 **   1) Sets the euid to the test user
 **   2) Forks; child calls ptrace with request=PTRACE_TRACEME
 **   3) Parent waits for child to terminate
 **   4) Sets the euid to the superuser
 **   5) Verifies the success result.
 **
 **  The successful case sets up all parameters in such a manner to
 **  assure success, according to the man page for ptrace.
 **  
 **  In the erroneous case, this function:
 **   1) Calls ptrace on pid=1 with a PTRACE_KILL request
 **      
 **  The erroneous case passes in an invalid pid for given request,
 **  causing an EPERM error condition.
 **
 **  HISTORY    :
 **    06/03 Originated by Michael A. Halcrow <mike@halcrow.us>
 **    10/03 Furthered by Kylene J. Smith <kylene@us.ibm.com>
 **    03/04 Added exp_errno variable by D. Kent Soper <dksoper@us.ibm.com>
 **    05/04 Updates to suppress compile warnings by Kimberly D. Simon <kdsimon@us.ibm.com>
 **
 **********************************************************************/

#include "includes.h"
#include "syscalls.h"
#include <sys/ptrace.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/wait.h>

typedef struct errnoAndReturnValue_s {
    int returnValue;
    int savedErrno;
} errnoAndReturnValue_t;

int test_ptrace(laus_data *dataPtr)
{

    int rc = 0;
    int exp_errno = EPERM;

    enum __ptrace_request request;
    pid_t pid;
    void *addr;
    void *data;

    //key_t shmkey;    // variables not needed?
    //int shmsize;
    //int shmflg;
    int shmid;

    errnoAndReturnValue_t *earv;
    int savedErrno;

    int placeHolder;

    // Set the syscall-specific data
    printf5("Setting laus_var_data.syscallData.code to %d\n", AUDIT_ptrace);
    dataPtr->laus_var_data.syscallData.code = AUDIT_ptrace;

  /**
   * Do as much setup work as possible right here
   */
    // Initialize IPC
    // Shared memory
    if ((rc = seteuid(dataPtr->msg_euid)) != 0) {
	printf1("Unable to seteuid to %i: errno=%i\n",
		dataPtr->msg_euid, errno);
	goto EXIT;
    }
    if ((shmid = shmget(IPC_PRIVATE, sizeof(errnoAndReturnValue_t),
			IPC_CREAT | 0x777)) == -1) {
	printf1("Error getting shared memory: errno=%i\n", errno);
	goto EXIT;		// TODO: Explicitely account for the fact that the semaphore has been created at this point
    }
    if ((rc = seteuid(0)) != 0) {
	printf1("Unable to seteuid to %i: errno=%i\n", 0, errno);
	goto EXIT;
    }
    pid = 0;
    addr = &placeHolder;
    data = &placeHolder;
    request = PTRACE_TRACEME;
    if (!dataPtr->successCase) {
	request = PTRACE_KILL;
	pid = 1;
    }
    // Set up audit argument buffer
    if ((rc = auditArg4(dataPtr,
			AUDIT_ARG_IMMEDIATE, sizeof(enum __ptrace_request),
			&request, AUDIT_ARG_IMMEDIATE, sizeof(int), &pid,
			AUDIT_ARG_POINTER, 0, addr, AUDIT_ARG_POINTER, 0,
			data)) != 0) {
	printf1("Error setting up audit argument buffer\n");
	goto EXIT;
    }
    // Do pre-system call work
    if ((rc = preSysCall(dataPtr)) != 0) {
	printf1("ERROR: pre-syscall setup failed (%d)\n", rc);
	goto EXIT;
    }
    // Execute system call
    if (dataPtr->successCase) {
	int pid;
	if ((pid = fork()) == 0) {
	    errnoAndReturnValue_t *childEarv;
	    // In child
	    if ((rc = seteuid(0)) != 0) {
		printf1("Unable to seteuid to %i: errno=%i\n",
			dataPtr->msg_euid, errno);
		goto EXIT;
	    }
	    if (((long)(childEarv = shmat(shmid, NULL, 0))) == -1) {
		printf1
		    ("Error attaching to shared memory segment with id %d: errno=%i\n",
		     shmid, errno);
		// TODO: Something a bit more drastic should happen at this point
		_exit(0);
	    }
	    if ((rc = seteuid(dataPtr->msg_euid)) != 0) {
		printf1("Unable to seteuid to %i: errno=%i\n",
			dataPtr->msg_euid, errno);
		goto EXIT;
	    }
	    childEarv->returnValue =
		syscall(__NR_ptrace, request, pid, addr, data);
	    childEarv->savedErrno = errno;
	    if ((rc = seteuid(0)) != 0) {
		printf1("Unable to seteuid to %i: errno=%i\n",
			dataPtr->msg_euid, errno);
		goto EXIT;
	    }
	    if (shmdt(childEarv) == -1) {
		printf1
		    ("Error detaching from shared memory segment at address 0x%p: errno=%i\n",
		     childEarv, errno);
		_exit(0);
	    }
	    _exit(0);
	} else {
	    // In parent
	    dataPtr->msg_pid = pid;
	    if (waitpid(pid, NULL, 0) == -1) {
		printf1("Error waiting on pid %d: errno=%i\n", pid, errno);
		goto EXIT_CLEANUP;
	    }
	    if ((rc = seteuid(0)) != 0) {
		printf1("Unable to seteuid to %i: errno=%i\n", 0, errno);
		goto EXIT;
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
    } else {
	dataPtr->laus_var_data.syscallData.result =
	    syscall(__NR_ptrace, request, pid, addr, data);
	savedErrno = errno;
    }

    // Do post-system call work
    if ((rc = postSysCall(dataPtr, savedErrno, -1, exp_errno)) != 0) {
	printf1("ERROR: post-syscall setup failed (%d)\n", rc);
	goto EXIT;
    }

EXIT_CLEANUP:
    // Release the shared memory
    if (shmctl(shmid, 0, IPC_RMID) == -1) {
	printf1("Error removing shared memory with id %d: errno=%i\n", shmid,
		errno);
	goto EXIT;
    }

EXIT:
    printf5("Returning from test\n");
    return rc;
}
