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
 *  test_ioperm.c
 *
 *  PURPOSE:
 *  Verify audit of changes to port input/output permissions.
 *
 *  SYSCALLS:
 *  ioperm()
 *
 *  TESTCASE: successful
 *  Set port permissions as root user.
 *
 *  TESTCASE: unsuccessful
 *  Attempt to set port permissions as test user.
 */

#include "includes.h"
#include "syscalls.h"

#define TEST_PORT_ADDRESS 0x282
#define TEST_PORT_PERMS 1
#define TEST_NUM_BYTES 1

int test_ioperm(struct audit_data *context, int variation, int success)
{
    int rc = 0;
    int exit;

    if (!success) {
	rc = seteuid_test();
	if (rc < 0)
	    goto exit;
	context_setexperror(context, EPERM);
    }

    rc = context_setidentifiers(context);
    if (rc < 0)
	goto exit_suid;

    context_setbegin(context);
    fprintf(stderr, "Attempting %s(%x, %x, %x)\n",
	    context->u.syscall.sysname, TEST_PORT_ADDRESS,
	    TEST_NUM_BYTES, TEST_PORT_PERMS);
    errno = 0;
    exit = syscall(context->u.syscall.sysnum, TEST_PORT_ADDRESS,
		   TEST_NUM_BYTES, TEST_PORT_PERMS);
    context_setend(context);
    context_setresult(context, exit, errno);

exit_suid:
    errno = 0;
    if (!success && (seteuid(0) < 0))
	fprintf(stderr, "Error: seteuid(): %s\n", strerror(errno));

exit:
    return rc;
}
