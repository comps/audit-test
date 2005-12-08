/*  (c) Copyright Hewlett-Packard Development Company, L.P., 2005
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
#include <asm/ipc.h>
#endif
#include <sys/sem.h>

#define TEST_NOTIMED 1
#define TEST_TIMED   2

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

    errno = 0;
    context_setbegin(context);
    exit = (op == TEST_TIMED) ?
	semtimedop(semid, &sops, nsems, &timeout) :
	semop(semid, &sops, nsems);
    context_setend(context);

    if (exit < 0) {
        context->success = 0;
        context->error = context->u.syscall.exit = -errno;
    } else {
        context->success = 1;
        context->u.syscall.exit = exit;
    }

exit_root:
    if (!success && seteuid(0) < 0)
	fprintf(stderr, "Error: seteuid(0): %s\n", strerror(errno));

exit_set:
    if (semctl(semid, nsems, IPC_RMID) < 0)
	fprintf(stderr, "Error: removing semaphore set: %s\n", strerror(errno));

exit:
    return rc;
}

int test_semop(struct audit_data *context, int variation, int success)
{
    return common_semop(context, TEST_NOTIMED, success);
}

int test_semtimedop(struct audit_data *context, int variation, int success)
{
    return common_semop(context, TEST_TIMED, success);
}
