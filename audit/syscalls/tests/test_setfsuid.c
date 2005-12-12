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
 *  TESTCASE: modify successful
 *  As root, attempt to set fsuid to test user's uid.
 *
 *  TESTCASE: nomodify successful
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

static int common_setfsuid_modify(struct audit_data *context)
{
    int rc = 0;
    int fsuid;
    uid_t pre_fsuid;
    int exit;

    fsuid = gettestuid();
    if (fsuid < 0) {
	rc = -1;
	goto exit;
    }

    rc = context_setidentifiers(context);
    if (rc < 0)
	goto exit;

    context_setbegin(context);
    fprintf(stderr, "Attempting %s(%x)\n", context->u.syscall.sysname, fsuid);
    exit = syscall(context->u.syscall.sysnum, fsuid);
    context_setend(context);
    context_setresult(context, exit, 0);

    /* Use a second call to setfsuid() to verify that the fsuid was
     * set; setfsuid() returns the previous fsuid if it sets
     * fsuid, and the current fsuid if it does not, which always tells
     * us the result of the previous operation. */
    pre_fsuid = setfsuid(fsuid);
    if (pre_fsuid != fsuid) {
	fprintf(stderr, 
		"Error: fsuid was not modified in operation as expected.\n");
	rc = -1;
    }

    /* make context reflect the fact that we explicitly set fsuid */
    context_setfsuid(context, fsuid);

    if (seteuid(0) < 0)
	fprintf(stderr, "Error: seteuid(): %s\n", strerror(errno));

exit:
    return rc;
}

static int common_setfsuid_nomodify(struct audit_data *context)
{
    int rc = 0;
    int testuid;
    uid_t fsuid, pre_fsuid;
    int exit;

    testuid = gettestuid();
    if (testuid < 0)
	goto exit;
    fsuid = testuid + 1;

    rc = seteuid(testuid);
    if (rc < 0)
	goto exit;

    rc = context_setidentifiers(context);
    if (rc < 0)
	goto exit_suid;

    context_setbegin(context);
    fprintf(stderr, "Attempting %s(%x)\n", context->u.syscall.sysname, fsuid);
    exit = syscall(context->u.syscall.sysnum, fsuid);
    context_setend(context);
    context_setresult(context, exit, 0);

    /* Use a second call to setfsuid() to verify that the fsuid was
     * not set; setfsuid() returns the previous fsuid if it sets
     * fsuid, and the current fsuid if it does not, which always tells
     * us the result of the previous operation. */
    pre_fsuid = setfsuid(fsuid);
    if (pre_fsuid == fsuid) {
	fprintf(stderr, 
		"Error: fsuid was unexpectedly modified in operation.\n");
	rc = -1;
    }

exit_suid:
    if (seteuid(0) < 0)
	fprintf(stderr, "Error: seteuid(): %s\n", strerror(errno));

exit:
    return rc;
}

int test_setfsuid(struct audit_data *context, int variation, int success)
{
    switch(variation) {
    case TESTSC_MODIFY:
	return common_setfsuid_modify(context);
    case TESTSC_NOMODIFY:
	return common_setfsuid_nomodify(context);
    default:
	fprintf(stderr, "Test variation [%i] unsupported for %s()\n",
		variation, context->u.syscall.sysname);
	return -1;
    }
}

int test_setfsuid32(struct audit_data *context, int variation, int success)
{
    switch(variation) {
    case TESTSC_MODIFY:
	return common_setfsuid_modify(context);
    case TESTSC_NOMODIFY:
	return common_setfsuid_nomodify(context);
    default:
	fprintf(stderr, "Test variation [%i] unsupported for %s()\n",
		variation, context->u.syscall.sysname);
	return -1;
    }
}
