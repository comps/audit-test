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
 *  test_shmget.c
 *
 *  PURPOSE:
 *  Verify audit of attempts to create/get identifiers for shared
 *  memory segments.
 *
 *  SYSCALLS:
 *  shmget()
 *
 *  TESTCASE: successful
 *  Get identifier for an existing shared memory segment.
 *
 *  TESTCASE: unsuccessful
 *  Attempt to create a new shared memory segment with the key of an
 *  existing shared memory segment (specifying IPC_EXCL).
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

int test_shmget(struct audit_data *context, int variation, int success)
{
    int rc = 0;
    int flags = S_IRWXU|IPC_CREAT;
    int shmid;
    int exit;

    errno = 0;
    rc = shmid = shmget(TEST_IPC_KEY, PAGE_SIZE, S_IRWXU|IPC_CREAT);
    if (rc < 0) {
        fprintf(stderr, "Error: can't create shared mem segment: %s\n",
                strerror(errno));
        goto exit;
    }
    fprintf(stderr, "shared mem segment key: %d id: %d\n", TEST_IPC_KEY, shmid);

    if (!success) {
        flags |= IPC_EXCL;
        context_setexperror(context, EEXIST);
    }

    rc = context_setidentifiers(context);
    if (rc < 0)
        goto exit_seg;

    context_setbegin(context);
    fprintf(stderr, "Attempting %s(%x, %lx, %x)\n", 
	    context->u.syscall.sysname, TEST_IPC_KEY, PAGE_SIZE, flags);
    errno = 0;
    /* using library routine makes it portable between arches */
    exit = shmget(TEST_IPC_KEY, PAGE_SIZE, flags);
    context_setend(context);
    context_setresult(context, exit, errno);

    fprintf(stderr, "shared mem segment id: %d\n", exit);

exit_seg:
    errno = 0;
    if (shmctl(shmid, IPC_RMID, NULL) < 0)
        fprintf(stderr, "Error: removing shared mem segment: %s\n", 
		strerror(errno));

exit:
    return rc;
}
