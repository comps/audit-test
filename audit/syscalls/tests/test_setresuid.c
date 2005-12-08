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
 *  test_setresuid.c
 *
 *  PURPOSE:
 *  Verify audit of attempts to set saved user identity.
 *
 *  SYSCALLS:
 *  setresuid(), setresuid32()
 *
 *  TESTCASE: successful 
 *  As root, attempt to set saved uid to test user's uid.  Real and
 *  effective uids are left unchanged.
 *
 *  TESTCASE: unsuccessful
 *  AS test user, attempt to set saved uid to test user's uid+1;
 *  Real and effective uids are left unchanged.
 */

#include "includes.h"
#include "syscalls.h"

static int common_setresuid(struct audit_data *context, int success)
{
    int rc = 0;
    int testuid;
    uid_t uid;
    int exit;

    testuid = gettestuid();
    if (testuid < 0) {
	rc = -1;
	goto exit;
    }
    uid = testuid;

    if (!success) {
	context_setexperror(context, EPERM);
	uid = testuid + 1;

	rc = seteuid(testuid);
	if (rc < 0)
	    goto exit;
    }

    context_setbegin(context);
    fprintf(stderr, "Attempting %s(%x, %x, %x)\n", 
	    context->u.syscall.sysname, -1, -1, uid);
    errno = 0;
    exit = syscall(context->u.syscall.sysnum, -1, -1, uid);
    context_setend(context);
    context_setresult(context, exit, errno);

    rc = context_setidentifiers(context);

    errno = 0;
    if (setresuid(0, 0, 0) < 0)
	fprintf(stderr, "Error: setresuid(): %s\n", strerror(errno));

exit:
    return rc;
}

int test_setresuid(struct audit_data *context, int variation, int success)
{
    return common_setresuid(context, success);
}

int test_setresuid32(struct audit_data *context, int variation, int success)
{
    return common_setresuid(context, success);
}
