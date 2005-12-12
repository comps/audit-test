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
 *  test_shmat.c
 *
 *  PURPOSE:
 *  Verify audit of attempts to attach to a shared memory segment.
 *
 *  SYSCALLS:
 *  shmat()
 *
 *  TESTCASE: remove successful
 *  Attach to an existing shared memory segment.
 *
 *  TESTCASE: remove unsuccessful
 *  Attempt to attach to an existing shared memory segment with
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

int test_shmat(struct audit_data *context, int variation, int success)
{
    int rc = 0;
    int shmid;
    void *shmadd;

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
        context_setexperror(context, EACCES);
    }

    rc = context_setidentifiers(context);
    if (rc < 0)
        goto exit_root;
#if defined (__i386)
    context_setarg(context, 0, SHMAT);
#endif

    context_setbegin(context);
#if defined (__x86_64) || defined (__ia64)
    fprintf(stderr, "Attempting %s(%x)\n", 
	    context->u.syscall.sysname, shmid);
#else
    fprintf(stderr, "Attempting %s(%x, %x)\n", 
	    context->u.syscall.sysname, SHMAT, shmid);
#endif
    errno = 0;
    shmadd = shmat(shmid, NULL, 0);
    context_setend(context);
    context_setresult(context, (long)shmadd, errno);

    errno = 0;
    if (((unsigned long)shmadd != (unsigned long)-1) && (shmdt(shmadd) < 0))
	fprintf(stderr, "Error: can't detach from shared memory: %s\n", 
		strerror(errno));

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
