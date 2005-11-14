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
  **  FILE       : test_execve.c
  **
  **  PURPOSE    : To test the execve library call auditing.
  **
  **  DESCRIPTION: The test_execve() function builds into the
  **  laus_test framework to verify that the Linux Audit System
  **  accurately logs both successful and erroneous execution of the
  **  "execve" system call.
  **
  **  We use IPC semaphores to coordinate the child and the parent
  **  processes.
  **  
  **  In the successful case, this function:
  **   1) Creates a new semaphore
  **   2) Forks a child process, which in turn runs execve() with
  **      semaphore_poster as the filename and the semaphore identifier
  **      as a command-line argument.  This process posts to the
  **      semaphore associated with the given identifier
  **   3) Spinlock pends on the semaphore
  **   4) Compares the result against the expected result for the
  **      success case.
  **
  **  On success, execve() does not return.  Hence, there is no return
  **  code.  We simply assign the value of ``0'' to the
  **  context->u.syscall.exit variable as a
  **  placeholder.  IPC machanisms are employed to detect the
  **  success (or lack therof) of the execve() call in the child
  **  process.
  **  
  **  In the erroneous case, this function:
  **   1) Calls execve() with a file that is not executable
  **   2) Compares the result against the expected result for the error
  **      case.
  **      
  **  The erroneous case causes execve() to flag an EACCES errno by
  **  passing it a non-executable filename, in 
  **  accordance with the man page's specification for error
  **  conditions.
  **
  **  HISTORY    :
  **    06/03 Originated by Michael A. Halcrow <mike@halcrow.us>
  **    03/04 Added exp_errno variable by D. Kent Soper <dksoper@us.ibm.com>
  **    05/04 Updates to suppress compiler warnings by Kimberly D. Simon <kdsimon@us.ibm.com>
  **
  **********************************************************************/

#include "includes.h"
#include "syscalls.h"
#include <sys/ipc.h>
#include <sys/sem.h>
#include <time.h>

int test_execve(struct audit_data *context)
{

    int rc = 0;
    int exp_errno = EACCES;

    int pid;

    char *filename = "../utils/semaphore_poster";
    char *filename2 = "../utils/semaphore_poster2";
    char path[PATH_MAX];
    //char* nullPtr = NULL;     // not needed?
    char *argv[3];
    char *envp[1];
    char *vector = NULL;
    int vector_size = 0;

    int semid = -1;

   /**
    * Do as much setup work as possible right here
    */
    envp[0] = NULL;
    if (context->success) {
	// Set up for success
	if (chdir(context->u.syscall.cwd) == -1) {
	    fprintf(stderr, "Error changing to working directory [%s]: errno=%i\n",
		    context->u.syscall.cwd, errno);
	    goto EXIT;
	}
	context->euid = 0;
	context->egid = 0;
	context->fsuid = 0;
	context->fsgid = 0;
	argv[0] = filename;
	argv[1] = (char *)malloc(MAX_ARG_SIZE);
	argv[2] = NULL;
	if ((semid = semget(IPC_PRIVATE, 1, 0)) == -1) {
	    fprintf(stderr, "Error creating semaphore: errno=%i\n", errno);
	    goto EXIT_CLEANUP;
	}
	fprintf(stderr, "Created new semaphore w/ semid = %d\n", semid);
	sprintf(argv[1], "%d", semid);
	vector_size =
	    arg_vector(&vector, vector_size, AUDIT_ARG_STRING, strlen(argv[0]),
		       argv[0]);
	vector_size =
	    arg_vector(&vector, vector_size, AUDIT_ARG_STRING, strlen(argv[1]),
		       argv[1]);
    } else {
	// Set up for error
	argv[0] = NULL;
    }

    if (context->success) {
	realpath(filename, path);
    } else {
	realpath(filename2, path);
	context->euid = context->fsuid = helper_uid;
    }

    if ((rc = auditArg3(context,
			AUDIT_ARG_PATH,
			strlen(path),
			path,
			AUDIT_ARG_VECTOR, vector_size, vector,
			AUDIT_ARG_POINTER, 0, envp)) != 0) {
	fprintf(stderr, "Error setting up audit argument buffer\n");
	goto EXIT_CLEANUP;
    }

    // Do pre-system call work
    if ((rc = preSysCall(context)) != 0) {
	fprintf(stderr, "ERROR: pre-syscall setup failed (%d)\n", rc);
	goto EXIT_CLEANUP;
    }

    if (context->success) {
	if ((pid = fork()) == 0) {
	    // We are in the child
	    // Execute system call successfully
	    if (execve(filename, argv, envp) == -1) {
		fprintf(stderr, "execve() error in success case: errno=%i\n", errno);
		// Parent will time out waiting for child
		_exit(-1);
	    }
	    // This process is either assimilated or terminated before we
	    // get to this point :-)
	    fprintf(stderr, "BORK BORK BORK\n");	// If this actually prints, then
	    // lots of people have lots of
	    // really bad issues to deal with
	    _exit(-1);
	} else {
	    // We are in the parent; pid = process ID of the child
	    // We must be guaranteed that the execve() call has been made
	    // Wait on semaphore for that indication
	    struct sembuf sembuf;
	    time_t timestamp;
	    sembuf.sem_num = 0;
	    sembuf.sem_op = -1;
	    sembuf.sem_flg = IPC_NOWAIT;
	    fprintf(stderr, "Waiting for semaphore from child process...\n");
	    timestamp = time(NULL);
	    // This should spin just a few hundred times
	    while (semop(semid, &sembuf, 1) == -1) {
		if ((time(NULL) - timestamp) > MAX_WAIT_TIME_FOR_CHILD) {
		    fprintf
			(stderr,
			 "Timed out while waiting for semaphore from child\n");
		    goto EXIT_CLEANUP;
		    break;
		}
	    }
	    fprintf(stderr, "Received semaphore\n");
	    // execve() really has no success return code; this is a placeholder
	    context->u.syscall.exit = NO_RETURN_CODE;
	    //audit message is posted from the forked process update the ptr
	    context->pid = pid;
	    context->sgid = 0;
	}
    } else {
	// Execute system call erroneously
	context->u.syscall.exit = syscall(__NR_execve, filename2, argv, envp);
    }

    // Do post-system call work
    if ((rc = postSysCall(context, errno, -1, exp_errno)) != 0) {
	fprintf(stderr, "ERROR: post-syscall setup failed (%d)\n", rc);
	goto EXIT_CLEANUP;
    }

EXIT_CLEANUP:
   /**
    * Do cleanup work here
    */
    if (context->success) {
	// Clean up from success case setup
	// Free command name buffer
	free(argv[1]);
	// Remove semaphore
	if (semctl(semid, 0, IPC_RMID) == -1) {
	    fprintf(stderr, "Error removing semaphore with semid=%d: errno=%i\n", semid,
		    errno);
	    goto EXIT;
	}
    }

EXIT:
    fprintf(stderr, "Returning from test\n");
    return rc;
}
