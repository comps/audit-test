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

int test_semctl_set(struct audit_data *context, int variation, int success)
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
        context_setexperror(context, EPERM);
    }

    rc = context_setidentifiers(context);
    if (rc < 0)
        goto exit_root;
#if defined (__x86_64) || defined (__ia64)
    context_setarg(context, 2, IPC_SET);
#else
    context_setarg(context, 0, SEMCTL);
    context_setarg(context, 3, IPC_SET | 0x100); /* unknown flag set in call */
#endif

    memset(&buf, 0, sizeof(buf));
    buf.sem_perm.uid = gettestuid();
    /* semid_ds does not have a qbytes field */
    context_setipc(context, 0, buf.sem_perm.uid, 
		   buf.sem_perm.gid, buf.sem_perm.mode);

    context_setbegin(context);
#if defined (__x86_64) || defined (__ia64)
    fprintf(stderr, "Attempting %s(%x, %x, %x, %p)\n", 
	    context->u.syscall.sysname, semid, nsems, IPC_SET, &buf);
#else
    fprintf(stderr, "Attempting %s(%x, %x, %x, %x, %p)\n", 
	    context->u.syscall.sysname, SEMCTL, semid, nsems, IPC_SET, &buf);
#endif
    errno = 0;
    exit = semctl(semid, nsems, IPC_SET, &buf);
    context_setend(context);
    context_setresult(context, exit, errno);

exit_root:
    errno = 0;
    if (!success && (seteuid(0) < 0))
	fprintf(stderr, "Error: seteuid(): %s\n", strerror(errno));

exit_set:
    errno = 0;
    if (semctl(semid, nsems, IPC_RMID, NULL) < 0)
	fprintf(stderr, "Error: removing semaphore set: %s\n", strerror(errno));

exit:
    return rc;
}

int test_semctl_rmid(struct audit_data *context, int variation, int success)
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
        context_setexperror(context, EPERM);
    }

    rc = context_setidentifiers(context);
    if (rc < 0)
        goto exit_root;
#if defined (__x86_64) || defined (__ia64)
    context_setarg(context, 2, IPC_RMID);
#else
    context_setarg(context, 0, SEMCTL);
    context_setarg(context, 3, IPC_RMID | 0x100); /* unknown flag set in call */
#endif

    context_setbegin(context);
#if defined (__x86_64) || defined (__ia64)
    fprintf(stderr, "Attempting %s(%x, %x, %x, %p)\n", 
	    context->u.syscall.sysname, semid, nsems, IPC_RMID, NULL);
#else
    fprintf(stderr, "Attempting %s(%x, %x, %x, %x, %p)\n", 
	    context->u.syscall.sysname, SEMCTL, semid, nsems, IPC_RMID, NULL);
#endif
    errno = 0;
    exit = semctl(semid, nsems, IPC_RMID, NULL);
    context_setend(context);
    context_setresult(context, exit, errno);

exit_root:
    errno = 0;
    if (!success && (seteuid(0) < 0))
	fprintf(stderr, "Error: seteuid(): %s\n", strerror(errno));

exit_set:
    errno = 0;
    if (exit < 0 && semctl(semid, nsems, IPC_RMID, NULL) < 0)
	fprintf(stderr, "Error: removing semaphore set: %s\n", strerror(errno));

exit:
    return rc;
}
