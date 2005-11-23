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
 *  test_semctl.c
 *
 *  PURPOSE:
 *  Verify audit of attempts to perform semaphore control operations.
 *
 *  SYSCALLS:
 *  semctl()
 *
 *  TESTCASE: remove successful
 *  Remove a semaphore set.
 *
 *  TESTCASE: remove unsuccessful
 *  Attempt to remove a semaphore set with insufficient access
 *  permissions.
 *
 *  TESTCASE: setperms successful
 *  Change semaphore set permissions.
 *
 *  TESTCASE: setperms unsuccessful
 *  Attempt to change semaphore set permissions, while having
 *  insufficient access permissions.
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

static int test_semctl_setperms(struct audit_data *context, int success)
{
    int rc = 0;
    int semid;
    int nsems = 1;
    struct semid_ds buf;
    int exit;

    errno = 0;
    rc = semid = semget(IPC_PRIVATE, nsems, S_IRWXU|IPC_CREAT);
    if (rc < 0) {
	fprintf(stderr, "Error: can't create semaphore set: %s\n",
		strerror(errno));
        goto exit;
    }
    fprintf(stderr, "Semaphore set key: %d id: %d\n", IPC_PRIVATE, semid);

    if (!success) {
	rc = seteuid_test();
	if (rc < 0)
	    goto exit_set;
        context->experror = -EPERM;
    }

    rc = context_setidentifiers(context);
    if (rc < 0)
        goto exit_root;

    memset(&buf, 0, sizeof(buf));
    buf.sem_perm.uid = gettestuid();
    /* semid_ds does not have a qbytes field */
    context_setipc(context, 0, buf.sem_perm.uid, 
		   buf.sem_perm.gid, buf.sem_perm.mode);

    errno = 0;
    context_setbegin(context);
    fprintf(stderr, "Attempting semctl(%d, %d, IPC_SET\n", semid, nsems);
    exit = semctl(semid, nsems, IPC_SET, &buf);
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
    if (semctl(semid, nsems, IPC_RMID, NULL) < 0)
	fprintf(stderr, "Error: removing semaphore set: %s\n", strerror(errno));

exit:
    return rc;
}

static int test_semctl_remove(struct audit_data *context, int success)
{
    int rc = 0;
    int semid;
    int nsems = 1;
    int exit = -1; /* pre-set for proper cleanup */

    errno = 0;
    rc = semid = semget(IPC_PRIVATE, nsems, S_IRWXU|IPC_CREAT);
    if (rc < 0) {
	fprintf(stderr, "Error: can't create semaphore set: %s\n",
		strerror(errno));
        goto exit;
    }
    fprintf(stderr, "Semaphore set key: %d id: %d\n", IPC_PRIVATE, semid);

    if (!success) {
	rc = seteuid_test();
	if (rc < 0)
	    goto exit_set;
        context->experror = -EPERM;
    }

    rc = context_setidentifiers(context);
    if (rc < 0)
        goto exit_root;

    errno = 0;
    context_setbegin(context);
    fprintf(stderr, "Attempting semctl(%d, %d, IPC_RMID)\n", semid, nsems);
    exit = semctl(semid, nsems, IPC_RMID, NULL);
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
    if (exit < 0 && semctl(semid, nsems, IPC_RMID, NULL) < 0)
	fprintf(stderr, "Error: removing semaphore set: %s\n", strerror(errno));

exit:
    return rc;
}

int test_semctl(struct audit_data *context, int variation, int success)
{
    switch(variation) {
    case SYSCALL_REMOVE:
	return test_semctl_remove(context, success);
    case SYSCALL_SETPERMS:
	return test_semctl_setperms(context, success);
    default:
	fprintf(stderr, "Test variation [%i] unsupported for %s()\n", 
		variation, context->u.syscall.sysname);
	return -1;
    }
}
