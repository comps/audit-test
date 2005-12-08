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
 *  test_swapon.c
 *
 *  PURPOSE:
 *  Verify audit of attempts to start swapping to a file/device.
 *
 *  SYSCALLS:
 *  swapon()
 *
 *  TESTCASE: successful
 *  Start swapping to a file as the super user.
 *
 *  TESTCASE: unsuccessful
 *  Attempt to start swapping to a file as an unprivileged user.
 */

#include "includes.h"
#include "syscalls.h"
#include <sys/swap.h>

int test_swapon(struct audit_data *context, int variation, int success)
{
    int rc = 0;
    char *path;
    int exit;

    path = init_tempswap(S_IRWXU, context->euid, context->egid, 1024 * 1024);
    if (!path) {
	rc = -1;
	goto exit;
    }

    if (!success) {
	rc = seteuid_test();
	if (rc < 0)
	    goto exit_path;
	context_setexperror(context, EPERM);
    }

    rc = context_setidentifiers(context);
    if (rc < 0)
	goto exit_suid;

    /* The swapfile isn't considered the object of the operation, and
     * does not reliably appear in the audit record, so don't test for
     * it here.
     */

    context_setbegin(context);
    fprintf(stderr, "Attempting %s(%s, %x)\n", 
	    context->u.syscall.sysname, path, 0);
    errno = 0;
    exit = syscall(context->u.syscall.sysnum, path, 0);
    context_setend(context);
    context_setresult(context, exit, errno);

    errno = 0;
    if ((exit != -1) && (swapoff(path) < 0))
	fprintf(stderr, "Error: swapoff(%s): %s\n", path, strerror(errno));

exit_suid:
    errno = 0;
    if (!success && (seteuid(0) < 0))
	fprintf(stderr, "Error: seteuid(): %s\n", strerror(errno));

exit_path:
    destroy_tempfile(path);

exit:
    return rc;
}
