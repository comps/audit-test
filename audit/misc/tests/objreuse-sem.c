/* objreuse-sem.c - semaphore allocated w/ semget() must be zeroed by kernel
 *
 * $Id: objreuse-sem.c,v 1.1 2004/06/23 22:48:21 danjones Exp $
 *
 * Purpose: verify that the object reuse mechanism works as documented
 * 	in the case of a semaphore returned by the kernel. All memory
 *	made available must contain zeroes and not any data that may
 *	from previous use.
 *
 * Method: allocate semaphore via the semget() system call and verify the
 *	contents are all initialized to zeros.
 *
 * Expected result: exit code 0 indidates that all tests worked as
 *	expected.
 *
 */

#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>
#include <stdarg.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/stat.h>

#include "testmacros.h"

int main(int argc, char *argv[])
{
	int semid, semval;

	int increments[]={
		1, 2, 3, 4, 15, 16, 17, 250
	};
	int i,j;

	for (i=0; i < sizeof(increments)/sizeof(increments[0]); i++) {
	  SYSCALL( semid = semget(IPC_PRIVATE, increments[i], S_IRUSR) );
	  for (j=0; j < increments[i]; j++) {
	    SYSCALL( semval = semctl(semid, j, GETVAL) );
	    DIE_IF( semval != 0 );
	  }
	  SYSCALL( semctl(semid, 0, IPC_RMID) );
	}

        printf("%s: PASS\n", __FILE__);
	return 0;
}
