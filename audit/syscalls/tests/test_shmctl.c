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
 *  test_shmctl.c
 *
 *  PURPOSE:
 *  Verify audit of attempts to perform shared memory control operations.
 *
 *  SYSCALLS:
 *  shmctl()
 *
 *  TESTCASE: remove successful
 *  Mark a shared memory segment as destroyed.
 *
 *  TESTCASE: remove unsuccessful
 *  Attempt to mark a shared memory segment as destroyed with
 *  insufficient access permissions.
 *
 *  TESTCASE: setperms successful
 *  Change shared memory segment permissions.
 *
 *  TESTCASE: setperms unsuccessful
 *  Attempt to change shared memory segment permissions, while having
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
#include <asm/page.h>
#include <sys/shm.h>

static int test_shmctl_setperms(struct audit_data *context, int success)
{
    int rc = 0;
    int shmid;
    struct shmid_ds buf;
    int exit;

    errno = 0;
    rc = shmid = shmget(IPC_PRIVATE, PAGE_SIZE, S_IRWXU|IPC_CREAT);
    if (rc < 0) {
	fprintf(stderr, "Error: can't create shared mem segment: %s\n",
		strerror(errno));
        goto exit;
    }
    fprintf(stderr, "shared mem segment key: %d id: %d\n", IPC_PRIVATE, shmid);

    if (!success) {
	rc = seteuid_test();
	if (rc < 0)
	    goto exit_seg;
        context->experror = -EPERM;
    }

    rc = context_setidentifiers(context);
    if (rc < 0)
        goto exit_root;

    memset(&buf, 0, sizeof(buf));
    buf.shm_perm.uid = gettestuid();
    /* shmid_ds does not have a qbytes field */
    context_setipc(context, 0, buf.shm_perm.uid, 
		   buf.shm_perm.gid, buf.shm_perm.mode);

    errno = 0;
    context_setbegin(context);
    fprintf(stderr, "Attempting shmctl(%d, IPC_SET)\n", shmid);
    exit = shmctl(shmid, IPC_SET, &buf);
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

exit_seg:
    if (shmctl(shmid, IPC_RMID, NULL) < 0)
	fprintf(stderr, "Error: removing shared mem segment: %s\n",
		strerror(errno));

exit:
    return rc;
}

static int test_shmctl_remove(struct audit_data *context, int success)
{
    int rc = 0;
    int shmid;
    int exit = -1; /* pre-set for proper cleanup */

    errno = 0;
    rc = shmid = shmget(IPC_PRIVATE, PAGE_SIZE, S_IRWXU|IPC_CREAT);
    if (rc < 0) {
	fprintf(stderr, "Error: can't create shared mem segment: %s\n",
		strerror(errno));
        goto exit;
    }
    fprintf(stderr, "shared mem segment key: %d id: %d\n", IPC_PRIVATE, shmid);

    if (!success) {
	rc = seteuid_test();
	if (rc < 0)
	    goto exit_seg;
        context->experror = -EPERM;
    }

    rc = context_setidentifiers(context);
    if (rc < 0)
        goto exit_root;

    errno = 0;
    context_setbegin(context);
    fprintf(stderr, "Attempting shmctl(%d, IPC_RMID)\n", shmid);
    exit = shmctl(shmid, IPC_RMID, NULL);
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

exit_seg:
    if (exit < 0 && shmctl(shmid, IPC_RMID, NULL) < 0)
	fprintf(stderr, "Error: removing shared mem segment: %s\n",
		strerror(errno));

exit:
    return rc;
}

int test_shmctl(struct audit_data *context, int variation, int success)
{
    switch(variation) {
    case SYSCALL_REMOVE:
	return test_shmctl_remove(context, success);
    case SYSCALL_SETPERMS:
	return test_shmctl_setperms(context, success);
    default:
	fprintf(stderr, "Test variation [%i] unsupported for %s()\n", 
		variation, context->u.syscall.sysname);
	return -1;
    }
}
