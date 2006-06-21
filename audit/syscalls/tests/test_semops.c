/*  (c) Copyright Hewlett-Packard Development Company, L.P., 2005
 *  Written by Amy Griffis <amy.griffis@hp.com>
 *
 *  This program is free software;  you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY;  without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See
 *  the GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program;  if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 *
 *  FILE:
 *  test_semops.c
 *
 *  PURPOSE:
 *  Verify audit of attempts to perform semaphore operations.
 *
 *  SYSCALLS:
 *  semop(), semtimedop()
 *
 *  TESTCASE: successful
 *  Perform a semaphore operation.
 *
 *  TESTCASE: unsuccessful
 *  Attempt to perform a semaphore operation with insufficient access
 *  permissions to the set.
 */

#include "includes.h"
#include "syscalls.h"
#include <sys/ipc.h>
#if defined(__powerpc64__)
#include <asm-ppc64/ipc.h>
#elif !defined(__ia64__)
#include <asm-generic/ipc.h>
#endif
#include <sys/sem.h>

/* SEMOP is not defined in ia64 headers, and 
 * SEMTIMEDOP is not defined in any headers. */
#define SEMOP      1
#define SEMTIMEDOP 4

static int common_semop(struct audit_data *context, int op, int success)
{
    int rc = 0;
    int semid;
    int nsems = 1;
    struct sembuf sops = { 0, 1, 0 }; /* semaphore: num, op, flag */
    struct timespec timeout = { 0, 0 };
    int exit;

    rc = semid = semget(IPC_PRIVATE, nsems, S_IRWXU|IPC_CREAT);
    if (rc < 0) {
	fprintf(stderr, "Error creating semaphore: errno=%i\n", errno);
	goto exit;
    }
    fprintf(stderr, "semaphore set key: %d id: %d\n", IPC_PRIVATE, semid);

    if (!success) {
        rc = seteuid_test();
        if (rc < 0)
            goto exit_set;
        context_setexperror(context, EACCES);
    }

    rc = context_setidentifiers(context);
    if (rc < 0)
        goto exit_root;
#if defined (__i386)
    context_setarg(context, 0, op);
#endif

    context_setbegin(context);
#if defined (__x86_64) || defined (__ia64)
    fprintf(stderr, "Attempting %s(%x, %p, %x, %p)\n", 
	    context->u.syscall.sysname, semid, &sops, nsems, 
	    (op == SEMTIMEDOP) ? &timeout : NULL);
#else
    fprintf(stderr, "Attempting %s(%x, %x, %p, %x, %p)\n", 
	    context->u.syscall.sysname,  op, semid, &sops, nsems, 
	    (op == SEMTIMEDOP) ? &timeout : NULL);
#endif
    errno = 0;
    exit = (op == SEMTIMEDOP) ?
	semtimedop(semid, &sops, nsems, &timeout) :
	semop(semid, &sops, nsems);
    context_setend(context);
    context_setresult(context, exit, errno);

exit_root:
    errno = 0;
    if (!success && (seteuid(0) < 0))
	fprintf(stderr, "Error: seteuid(): %s\n", strerror(errno));

exit_set:
    errno = 0;
    if (semctl(semid, nsems, IPC_RMID) < 0)
	fprintf(stderr, "Error: removing semaphore set: %s\n", strerror(errno));

exit:
    return rc;
}

int test_semop(struct audit_data *context, int variation, int success)
{
    return common_semop(context, SEMOP, success);
}

int test_semtimedop(struct audit_data *context, int variation, int success)
{
    return common_semop(context, SEMTIMEDOP, success);
}
