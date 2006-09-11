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
#include <asm-generic/ipc.h>
#endif
#include <sys/shm.h>

int test_shmctl_set(struct audit_data *context, int variation, int success)
{
    int rc = 0;
    int shmid;
    struct shmid_ds buf;
    int exit;

    errno = 0;
    rc = shmid = shmget(IPC_PRIVATE, getpagesize(), S_IRWXU|IPC_CREAT);
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
        context_setexperror(context, EPERM);
    }

    rc = context_setidentifiers(context);
    if (rc < 0)
        goto exit_root;
#if defined (__x86_64) || defined (__ia64)
    context_setarg(context, 1, IPC_SET);
#else
    context_setarg(context, 0, SHMCTL);
    context_setarg(context, 2, IPC_SET | 0x100); /* unknown flag set in call */
#endif

    memset(&buf, 0, sizeof(buf));
    buf.shm_perm.uid = gettestuid();
    /* shmid_ds does not have a qbytes field */
    context_setipc(context, 0, buf.shm_perm.uid, 
		   buf.shm_perm.gid, buf.shm_perm.mode);

    context_setbegin(context);
#if defined (__x86_64) || defined (__ia64)
    fprintf(stderr, "Attempting %s(%x, %x, %p)\n", 
	    context->u.syscall.sysname, shmid, IPC_SET, &buf);
#else
    fprintf(stderr, "Attempting %s(%x, %x, %x, %p)\n", 
	    context->u.syscall.sysname, SHMCTL, shmid, IPC_SET, &buf);
#endif
    errno = 0;
    exit = shmctl(shmid, IPC_SET, &buf);
    context_setend(context);
    context_setresult(context, exit, errno);

exit_root:
    errno = 0;
    if (!success && (seteuid(0) < 0))
	fprintf(stderr, "Error: seteuid(): %s\n", strerror(errno));

exit_seg:
    errno = 0;
    if (shmctl(shmid, IPC_RMID, NULL) < 0)
	fprintf(stderr, "Error: removing shared mem segment: %s\n",
		strerror(errno));

exit:
    return rc;
}

int test_shmctl_rmid(struct audit_data *context, int variation, int success)
{
    int rc = 0;
    int shmid;
    int exit = -1; /* pre-set for proper cleanup */

    errno = 0;
    rc = shmid = shmget(IPC_PRIVATE, getpagesize(), S_IRWXU|IPC_CREAT);
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
        context_setexperror(context, EPERM);
    }

    rc = context_setidentifiers(context);
    if (rc < 0)
        goto exit_root;
#if defined (__x86_64) || defined (__ia64)
    context_setarg(context, 1, IPC_RMID);
#else
    context_setarg(context, 0, SHMCTL);
    context_setarg(context, 2, IPC_RMID | 0x100); /* unknown flag set in call */
#endif

    context_setbegin(context);
#if defined (__x86_64) || defined (__ia64)
    fprintf(stderr, "Attempting %s(%x, %x, %p)\n", 
	    context->u.syscall.sysname, shmid, IPC_RMID, NULL);
#else
    fprintf(stderr, "Attempting %s(%x, %x, %x, %p)\n", 
	    context->u.syscall.sysname, SHMCTL, shmid, IPC_RMID, NULL);
#endif
    errno = 0;
    exit = shmctl(shmid, IPC_RMID, NULL);
    context_setend(context);
    context_setresult(context, exit, errno);

exit_root:
    errno = 0;
    if (!success && (seteuid(0) < 0))
	fprintf(stderr, "Error: seteuid(): %s\n", strerror(errno));

exit_seg:
    errno = 0;
    if ((exit < 0) && (shmctl(shmid, IPC_RMID, NULL) < 0))
	fprintf(stderr, "Error: removing shared mem segment: %s\n",
		strerror(errno));

exit:
    return rc;
}
