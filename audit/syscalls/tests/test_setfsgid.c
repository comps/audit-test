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
 *  test_setfsgid.c
 *
 *  PURPOSE:
 *  Verify audit of attempts to set filesystem group identity.
 *
 *  SYSCALLS:
 *  setfsgid(), setfsgid32()
 *
 *  TESTCASE: successful 
 *  As root, attempt to set fsgid to test user's gid.
 *
 *  TESTCASE: unsuccessful
 *  As test user with test user gids, attempt to set fsgid to root's gid.
 *
 *  NOTES:
 *  A process's fsgid is equivalent to its egid, unless 
 *  explicitly set by setfsgid().
 *
 *  setfsgid() fails when the caller's euid is not root, and the
 *  specified fsgid does not match the caller's current real gid,
 *  egid, sgid or fsgid.
 *
 *  The setfsgid() syscalls do not return any error codes.
 */

#include "includes.h"
#include "syscalls.h"

static int common_setfsgid(struct audit_data *context, int success)
{
    int rc = 0;
    int testgid;
    gid_t fsgid, pre_fsgid;
    int exit;

    testgid = gettestgid();
    if (testgid < 0) {
	rc = -1;
	goto exit;
    }
    fsgid = testgid;

    /* To produce failure case, switch to test user 
     * and attempt to set fsgid to root gid */
    if (!success) {
	/* no expected error */
	fsgid = 0;

	rc = setuidresgid_test();
	if (rc < 0)
	    goto exit;
    }

    context_setbegin(context);
    exit = syscall(context->u.syscall.sysnum, fsgid);
    context_setend(context);

    fprintf(stderr, "setfsgid(%d) returned %d\n", fsgid, exit);

    rc = context_setidentifiers(context);
    if (rc < 0)
	goto exit;

    /* On success, setfsgid() returns the previous value of fsgid.  
     * On error, setfsgid() returns the current value of fsgid.  This
     * interface requires a second call to determine whether the first
     * call was successful. */
    pre_fsgid = setfsgid(testgid);
    fprintf(stderr, "setfsgid(%d) returned %d\n", testgid, pre_fsgid);

    context->success = (pre_fsgid == fsgid);
    context->u.syscall.exit = exit;
    /* fsgid was set explicitly, so override the value from
     * context_setidentifiers() */
    context->fsgid = pre_fsgid;

exit:
    if (!success)
	rc = setuidresgid_root();
    return rc;
}

int test_setfsgid(struct audit_data *context, int variation, int success)
{
    return common_setfsgid(context, success);
}

int test_setfsgid32(struct audit_data *context, int variation, int success)
{
    return common_setfsgid(context, success);
}
