/*  Copyright (C) International Business Machines  Corp., 2003
 *  (c) Copyright Hewlett-Packard Development Company, L.P., 2005
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
 *  Implementation written by HP, based on original code from IBM.
 *
 *  FILE:
 *  test_semget.c
 *
 *  PURPOSE:
 *  Verify audit of attempts to create/get semaphore set identifiers.
 *
 *  SYSCALLS:
 *  semget()
 *
 *  TESTCASE: successful
 *  Get identifier for an existing semaphore set.
 *
 *  TESTCASE: unsuccessful
 *  Attempt to create a new semaphore set with the key of an existing
 *  semaphore set (specifying IPC_EXCL).
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

int test_semget(struct audit_data *context, int variation, int success)
{
    int rc = 0;
    int flag = S_IRWXU|IPC_CREAT;
    int semid;
    int nsems = 1;
    int exit;

    errno = 0;
    rc = semid = semget(TEST_IPC_KEY, nsems, S_IRWXU|IPC_CREAT);
    if (rc < 0) {
        fprintf(stderr, "Error: can't create semaphore set: %s\n",
                strerror(errno));
        goto exit;
    }
    fprintf(stderr, "Semaphore set key: %d id: %d\n", TEST_IPC_KEY, semid);

    if (!success) {
        flag |= IPC_EXCL;
        context->experror = -EEXIST;
    }

    rc = context_setidentifiers(context);
    if (rc < 0)
        goto exit_set;

    errno = 0;
    context_setbegin(context);
    fprintf(stderr, "Attempting semget(%d, %d, %d)\n", TEST_IPC_KEY, nsems, 
            flag);
    /* using library routine makes it portable between arches */
    exit = semget(TEST_IPC_KEY, nsems, flag);
    context_setend(context);

    fprintf(stderr, "Semaphore set id: %d [%d]\n", exit, errno);

    if (exit < 0) {
        context->success = 0;
        context->error = context->u.syscall.exit = -errno;
    } else {
        context->success = 1;
        context->u.syscall.exit = exit;
    }

exit_set:
    if (semctl(semid, nsems, IPC_RMID, 0) < 0)
        fprintf(stderr, "Error: removing semaphore set: %s\n", strerror(errno));

exit:
    return rc;
}
