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
 *  test_iopl.c
 *
 *  PURPOSE:
 *  Verify audit of changes to process I/O privilege level.
 *
 *  SYSCALLS:
 *  iopl()
 *
 *  TESTCASE: successful
 *  Set I/O privilege level as root user.
 *
 *  TESTCASE: unsuccessful
 *  Attempt to set I/O privilege level as test user.
 */

#include "includes.h"
#include "syscalls.h"

int test_iopl(struct audit_data *context)
{
    int rc = 0;
    int success = context->success; /* save intended result */
    int exit;

    /* To produce failure, attempt to set port perms as unprivileged user */
    if (!success) {
	rc = seteuid_test();
	if (rc < 0)
	    goto exit;
	context->experror = -EPERM;
    }

    rc = context_setidentifiers(context);
    if (rc < 0)
	goto exit;

    fprintf(stderr, "Attempt to set I/O privilege level to: 1\n");

    errno = 0;
    context_setbegin(context);
    exit = syscall(context->u.syscall.sysnum, 1);
    context_setend(context);

    if (exit < 0) {
	context->success = 0;
	context->u.syscall.exit = context->error = -errno;
    } else {
	context->success = 1;
	context->u.syscall.exit = exit;

	errno = 0;
	rc = syscall(context->u.syscall.sysnum, 0);
	if (rc < 0)
	    fprintf(stderr, "Error: iopl(): %s\n", strerror(errno));
    }

exit:
    if (!success)
	seteuid(0); /* clean up from failure case */
    return rc;
}
