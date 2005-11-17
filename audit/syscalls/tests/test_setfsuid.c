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
 *  test_setfsuid.c
 *
 *  PURPOSE:
 *  Verify audit of attempts to set filesystem user identity.
 *
 *  SYSCALLS:
 *  setfsuid(), setfsuid32()
 *
 *  TESTCASE: successful 
 *  As root, attempt to set fsuid to test user's uid.
 *
 *  TESTCASE: unsuccessful
 *  As test user attempt to set fsuid to test user's uid+1;
 *
 *  NOTES:
 *  A process's fsuid is equivalent to its euid, unless 
 *  explicitly set by setfsuid().
 *
 *  setfsuid() fails when the caller's euid is not root, and the
 *  specified fsuid does not match the caller's current real uid,
 *  euid, suid or fsuid.
 *
 *  The setfsuid() syscalls do not return any error codes.
 */

#include "includes.h"
#include "syscalls.h"

static int common_setfsuid(struct audit_data *context)
{
    int rc = 0;
    int success = context->success; /* save intended result */
    int testuid;
    uid_t fsuid, pre_fsuid;
    int exit;

    testuid = gettestuid();
    if (testuid < 0) {
	rc = -1;
	goto exit;
    }
    fsuid = testuid;

    /* To produce failure case, switch to test user 
     * and attempt to set fsuid to a value different from real
     * uid, euid, suid, or current fsuid. */
    if (!success) {
	/* no expected error */
	fsuid = testuid + 1;

	rc = seteuid(testuid);
	if (rc < 0)
	    goto exit;
    }

    context_setbegin(context);
    exit = syscall(context->u.syscall.sysnum, fsuid);
    context_setend(context);

    fprintf(stderr, "setfsuid(%d) returned %d\n", fsuid, exit);

    rc = context_setidentifiers(context);
    if (rc < 0)
	goto exit;

    /* On success, setfsuid() returns the previous value of fsuid.  
     * On error, setfsuid() returns the current value of fsuid.  This
     * interface requires a second call to determine whether the first
     * call was successful. */
    pre_fsuid = setfsuid(testuid);
    fprintf(stderr, "setfsuid(%d) returned %d\n", testuid, pre_fsuid);

    context->success = (pre_fsuid == fsuid);
    context->u.syscall.exit = exit;
    /* fsuid was set explicitly, so override the value from
     * context_setidentifiers() */
    context->fsuid = pre_fsuid;

exit:
    seteuid(0); /* always clean up */
    fprintf(stderr, "seteuid(0) returned %d\n", rc);
    return rc;
}

int test_setfsuid(struct audit_data *context)
{
    return common_setfsuid(context);
}

int test_setfsuid32(struct audit_data *context)
{
    return common_setfsuid(context);
}
