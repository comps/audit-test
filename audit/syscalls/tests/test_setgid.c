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
 *  test_setgid.c
 *
 *  PURPOSE:
 *  Verify audit of attempts to set effective group identity.
 *
 *  SYSCALLS:
 *  setgid(), setgid32()
 *
 *  TESTCASE: successful 
 *  As root, attempt to set egid to test user's gid.
 *
 *  TESTCASE: unsuccessful
 *  As test user with test user gids, attempt to set egid to root's gid.
 */

#include "includes.h"
#include "syscalls.h"

static int common_setgid(struct audit_data *context, int success)
{
    int rc = 0;
    int testgid;
    gid_t gid;
    int exit;

    testgid = gettestgid();
    if (testgid < 0) {
	rc = -1;
	goto exit;
    }
    gid = testgid;

    if (!success) {
	context_setexperror(context, EPERM);
	gid = 0;

	rc = setuidresgid_test();
	if (rc < 0)
	    goto exit;
    }

    context_setbegin(context);
    fprintf(stderr, "Attempting %s(%x)\n", context->u.syscall.sysname, gid);
    errno = 0;
    exit = syscall(context->u.syscall.sysnum, gid);
    context_setend(context);
    context_setresult(context, exit, errno);

    rc = context_setidentifiers(context);

    if (!success)
	setuidresgid_root();

exit:
    return rc;
}

int test_setgid(struct audit_data *context, int variation, int success)
{
    return common_setgid(context, success);
}

int test_setgid32(struct audit_data *context, int variation, int success)
{
    return common_setgid(context, success);
}
