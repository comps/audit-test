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
 **  FILE       : test_semop.c
 **
 **  PURPOSE    : To test the semop library call auditing.
 **
 **  DESCRIPTION: The test_semop() function builds into the laus_test
 **  framework to verify that the Linux Audit System accurately logs
 **  both successful and erroneous execution of the "lpc" system call.
 **
 **  In the successful case, this function:
 **   1) Sets the semaphore mode flags
 **   2) Clears the audit trail
 **   3) Sets key to IPC_PRIVATE
 **   4) Executes the "semop" library call
 **   5) Tests the result of the call against the expected successful
 **      return
 **   6) Deallocates the newly allocated semaphore.
 **  
 **  The successful case executes the expected conditions
 **  described by the "semop" library call man page.  That is,
 **  the semop() function is called using IPC_PRIVATE for the key
 **  value.  "mode" is set to ( S_IRWXU | S_IRWXG | S_IRWXO ).  "nsems"
 **  is set to 1.  The function performs an operation on the semaphore.
 **
 **  In the erroneous case, this function:
 **   1) Sets the semaphore mode flags
 **   2) Clears the audit trail
 **   3) Attempts to perform an operation on a semaphore that the test
 **      user cannot access. 
 **
 **  The erroneous case forces an error condition due to the test user
 **  does not have permission to perform operations upon the semaphore.
 **  This should cause an EACCES error.
 **
 **  HISTORY    :
 **    06/03 Originated by Michael Halcrow <mike@halcrow.us>
 **    06/03 Furthered by Dustin Kirkland (k1rkland@us.ibm.com)
 **    10/03 Adapted to invoke EACCES errno by Michael A. Halcrow
 **          <mike@halcrow.us>
 **    03/04 Added exp_errno variable by D. Kent Soper <dksoper@us.ibm.com>
 **
 **********************************************************************/

#include "includes.h"
#include "syscalls.h"
#include <sys/ipc.h>
#include <sys/sem.h>
#if defined(__PPC64)
#include <asm-ppc64/ipc.h>
#elif !defined(__IA64)
#include <asm/ipc.h>
#endif

int test_semop(struct audit_data *context)
{
    int rc = 0;
    int exp_errno = EACCES;
    int semid = -1;
    int mode;
    int nsems = 1;
    struct sembuf s;
    unsigned int nsops = 1;

    // Set the key value.
    // If successCase == 0, then we will be creating a semaphore set
    // under the permissions of root, which the test user will not have
    // permission to access.
    if (context->success) {
	mode = (IPC_CREAT | IPC_EXCL) | 0666;
	if ((semid = semget(IPC_PRIVATE, 1, mode)) == -1) {
	    printf1("Error creating semaphore: errno=%i\n", errno);
	    goto EXIT_CLEANUP;
	}
    } else {
	mode = 0000 | IPC_CREAT;
	if ((semid = semget(semid, nsems, mode)) == -1) {
	    printf1
		("Cannot create the semaphore set with key = -1: errno = [%i]\n",
		 errno);
	    goto EXIT;
	}
    }

    s.sem_num = 0;
    s.sem_op = 1;
    s.sem_flg = 0;


    // Set up audit argument buffer
    if ((rc = auditArg3(context,
			AUDIT_ARG_IMMEDIATE, sizeof(int), &semid,
			AUDIT_ARG_POINTER, sizeof(s), &s,
			AUDIT_ARG_IMMEDIATE, sizeof(int), &nsops)) != 0) {
	printf1("Error setting up audit argument buffer\n");
	goto EXIT;
    }
    // Do pre-system call work   
    preSysCall(context);

    // Execute the semop system call
#if (defined(__X86_64) || defined(__IA64)) && !defined(__MODE_32)
    context->u.syscall.exit =
	syscall(__NR_semop, semid, &s, 1);
#else
    context->u.syscall.exit =
	syscall(__NR_ipc, SEMOP, semid, 1, 0, &s);
//  context->u.syscall.exit = semop( semid, &s, 1 );
#endif

    // Do post-system call work
    postSysCall(context, errno, -1, exp_errno);

EXIT_CLEANUP:

    if (semid && (semid != -1)) {
	if ((semctl(semid, 0, IPC_RMID)) == -1) {
	    printf1
		("ERROR: Cannot deallocate message memory with semid=[%d]: errno=[%i]\n",
		 semid, errno);
	}
    }

EXIT:
    return rc;
}
