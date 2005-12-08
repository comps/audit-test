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
 *  test_setresgid.c
 *
 *  PURPOSE:
 *  Verify audit of attempts to set saved group identity.
 *
 *  SYSCALLS:
 *  setresgid(), setresgid32()
 *
 *  TESTCASE: successful 
 *  As root, attempt to set saved gid to test user's gid.  Real and
 *  effective gids are left unchanged.
 *
 *  TESTCASE: unsuccessful
 *  AS test user with test user gids, attempt to set saved gid to
 *  root's gid.  Real and effective gids are left unchanged.
 */

#include "includes.h"
#include "syscalls.h"

static int common_setresgid(struct audit_data *context, int success)
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

    /* To produce failure case, switch to test user and 
     * attempt to set saved gid to root's gid. */
    if (!success) {
	context_setexperror(context, EPERM);
	gid = 0;

	rc = setuidresgid_test();
	if (rc < 0)
	    goto exit;
    }

    errno = 0;
    context_setbegin(context);
    fprintf(stderr, "Attempting %s(%x, %x, %x)\n",
	    context->u.syscall.sysname, -1, -1, gid);
    exit = syscall(context->u.syscall.sysnum, -1, -1, gid);
    context_setend(context);
    context_setresult(context, exit, errno);

    rc = context_setidentifiers(context);

exit:
    if (!success)
	rc = setuidresgid_root();
    return rc;
}

int test_setresgid(struct audit_data *context, int variation, int success)
{
    return common_setresgid(context, success);
}

int test_setresgid32(struct audit_data *context, int variation, int success)
{
    return common_setresgid(context, success);
}
