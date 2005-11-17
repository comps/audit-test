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
 *  test_setreuid.c
 *
 *  PURPOSE:
 *  Verify audit of attempts to set real user identity.
 *
 *  SYSCALLS:
 *  setreuid(), setreuid32()
 *
 *  TESTCASE: successful 
 *  As root, attempt to set real uid to test user's uid.  
 *  Effective uid is left unchanged.
 *
 *  TESTCASE: unsuccessful
 *  As test user, attempt to set real uid to test user's uid+1;
 *  Effective uid is left unchanged.
 */

#include "includes.h"
#include "syscalls.h"

static int common_setreuid(struct audit_data *context)
{
    int rc = 0;
    int success = context->success; /* save intended result */
    int testuid;
    uid_t uid;
    int exit;

    testuid = gettestuid();
    if (testuid < 0) {
	rc = -1;
	goto exit;
    }
    uid = testuid;

    /* To produce failure case, switch to test user and 
     * attempt to set saved uid to a value different from any of the
     * current process user ids */
    if (!success) {
	context->experror = EPERM;
	uid = testuid + 1;

	rc = seteuid(testuid);
	if (rc < 0)
	    goto exit;
    }

    context_setbegin(context);
    errno = 0;
    exit = syscall(context->u.syscall.sysnum, uid, -1);
    context_setend(context);

    fprintf(stderr, "setreuid(%d, -1) returned %d\n", uid, exit);

    rc = context_setidentifiers(context);
    if (rc < 0)
	goto exit;

    if (exit < 0) {
	context->success = 0;
	context->u.syscall.exit = context->error = errno;
    } else {
	context->success = 1;
	context->u.syscall.exit = exit;
    }

exit:
    rc = setresuid(0, 0, 0); /* always clean up */
    fprintf(stderr, "setresuid(0, 0, 0) returned %d\n", rc);
    return rc;
}

int test_setreuid(struct audit_data *context)
{
    return common_setreuid(context);
}

int test_setreuid32(struct audit_data *context)
{
    return common_setreuid(context);
}
