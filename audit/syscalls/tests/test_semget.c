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
 **  FILE       : test_semget.c
 **
 **  PURPOSE    : To test the semget library call auditing.
 **
 **  DESCRIPTION: The test_semget() function builds into the laus_test
 **  framework to verify that the Linux Audit System accurately logs
 **  both successful and erroneous execution of the "lpc" system call.
 **
 **  In the successful case, this function:
 **   1) Sets the semaphore mode flags
 **   2) Clears the audit trail
 **   3) Sets key to IPC_PRIVATE
 **   4) Executes the "semget" library call
 **   5) Tests the result of the call against the expected successful
 **      return
 **   6) Deallocates the newly allocated semaphore.
 **  
 **  The successful case executes the expected conditions
 **  described by the "semget" library call man page.  That is,
 **  the semget() function is called using IPC_PRIVATE for the key
 **  value.  "mode" is set to ( S_IRWXU | S_IRWXG | S_IRWXO ).  "nsems"
 **  is set to 1.  The function returns an integer value specifying the
 **  semaphore identifier for the newly allocated semaphore; the
 **  identifier should be a valid value (not NULL or -1).
 **
 **  In the erroneous case, this function:
 **
 **  Semphore operations are attempted as a non-root user, thus
 **  causing an EACCES errno.
 ** 
 **  HISTORY    :
 **    06/03 Originated by Michael Halcrow <mike@halcrow.us>
 **    06/03 Furthered by Dustin Kirkland (k1rkland@us.ibm.com)
 **    10/03 Adapted to invoke EACCES errno by Michael A. Halcrow
 **          <mike@halcrow.us>
 **    03/04 Added exp_errno variable by D. Kent Soper <dksoper@us.ibm.com>
 **    05/04 Updates to suppress compile warnings by Kimberly D. Simon <kdsimon@us.ibm.com>
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
#include <sys/sem.h>

int test_semget(struct audit_data *context)
{
    int rc = 0;
    int exp_errno = EACCES;
    int key = 0;
    int firstSemid = 0;
    int secondSemid = 0;
    //int doNotDeallocate = 0;   // not needed?
    int mode;
    int nsems = 1;

    // Set the mode flags
    mode = S_IRWXU | S_IRWXG | S_IRWXO;

    // Set the number of semaphores for the identifier
    nsems = 1;

    // Set the key value.
    // If successCase == 0, then we will be creating a semaphore set
    // under the permissions of root, which the test user will not have
    // permission to access.
    if (context->success) {
	key = IPC_PRIVATE;
    } else {
	mode = 0600 | IPC_CREAT;
	key = -1;
	if ((firstSemid = semget(key, nsems, mode)) == -1) {
	    fprintf
		(stderr,
		 "Cannot create the semaphore set with key = -1: errno = [%i]\n",
		 errno);
	    goto EXIT;
	}
    }

    // Set up audit argument buffer
    if ((rc = auditArg3(context,
			AUDIT_ARG_IMMEDIATE, sizeof(int), &key,
			AUDIT_ARG_IMMEDIATE, sizeof(int), &nsems,
			AUDIT_ARG_IMMEDIATE, sizeof(int), &mode)) != 0) {
	fprintf(stderr, "Error setting up audit argument buffer\n");
	goto EXIT;
    }
    // Do pre-system call work   
    preSysCall(context);

    // Execute the semget system call
#if (defined(__x86_64__) || defined(__ia64__))
    context->u.syscall.exit = secondSemid =
	syscall(__NR_semget, key, nsems, mode);
#else
    context->u.syscall.exit = secondSemid =
	syscall(__NR_ipc, SEMGET, key, nsems, mode);
#endif

    // Do post-system call work
    postSysCall(context, errno, -1, exp_errno);

    //EXIT_FREE_SEM:   // not needed?

    if (firstSemid && (firstSemid != -1)) {
	if ((semctl(firstSemid, 0, IPC_RMID)) == -1) {
	    fprintf
		(stderr,
		 "ERROR: Cannot deallocate message memory with semid=[%d]: errno=[%i]\n",
		 firstSemid, errno);
	}
    }
    if (secondSemid && (secondSemid != -1)) {
	if ((semctl(secondSemid, 0, IPC_RMID)) == -1) {
	    fprintf
		(stderr,
		 "ERROR: Cannot deallocate message memory with semid=[%d]: errno=[%i]\n",
		 secondSemid, errno);
	    goto EXIT;
	}
    }

EXIT:
    return rc;
}
