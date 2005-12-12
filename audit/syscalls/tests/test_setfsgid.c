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
 *  TESTCASE: modify successful
 *  As root, attempt to set fsgid to test user's gid.
 *
 *  TESTCASE: nomodify successful
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

static int common_setfsgid_modify(struct audit_data *context)
{
    int rc = 0;
    int fsgid;
    gid_t pre_fsgid;
    int exit;

    fsgid = gettestgid();
    if (fsgid < 0) {
	rc = -1;
	goto exit;
    }

    rc = context_setidentifiers(context);
    if (rc < 0)
	goto exit;

    context_setbegin(context);
    fprintf(stderr, "Attempting %s(%x)\n", context->u.syscall.sysname, fsgid);
    exit = syscall(context->u.syscall.sysnum, fsgid);
    context_setend(context);
    context_setresult(context, exit, 0);

    /* Use a second call to setfsgid() to verify that the fsgid was
     * set; setfsgid() returns the previous fsgid if it sets
     * fsgid, and the current fsgid if it does not, which always tells
     * us the result of the previous operation. */
    pre_fsgid = setfsgid(fsgid);
    if (pre_fsgid != fsgid) {
	fprintf(stderr, 
		"Error: fsgid was not modified in operation as expected.\n");
	rc = -1;
    }

    /* make context reflect the fact that we explicitly set fsgid */
    context_setfsgid(context, fsgid);

    if (setegid(0) < 0)
	fprintf(stderr, "Error: setegid(): %s\n", strerror(errno));

exit:
    return rc;
}

static int common_setfsgid_nomodify(struct audit_data *context)
{
    int rc = 0;
    gid_t fsgid = 0;
    gid_t pre_fsgid;
    int exit;

    rc = setuidresgid_test();
    if (rc < 0)
	goto exit;

    rc = context_setidentifiers(context);
    if (rc < 0)
	goto exit_suid;

    context_setbegin(context);
    fprintf(stderr, "Attempting %s(%x)\n", context->u.syscall.sysname, fsgid);
    exit = syscall(context->u.syscall.sysnum, fsgid);
    context_setend(context);
    context_setresult(context, exit, 0);

    /* Use a second call to setfsgid() to verify that the fsgid was
     * not set; setfsgid() returns the previous fsgid if it sets
     * fsgid, and the current fsgid if it does not, which always tells
     * us the result of the previous operation. */
    pre_fsgid = setfsgid(fsgid);
    if (pre_fsgid == fsgid) {
	fprintf(stderr, 
		"Error: fsgid was unexpectedly modified in operation.\n");
	rc = -1;
    }

exit_suid:
    setuidresgid_root(); 

exit:
    return rc;
}

int test_setfsgid(struct audit_data *context, int variation, int success)
{
    switch(variation) {
    case TESTSC_MODIFY:
	return common_setfsgid_modify(context);
    case TESTSC_NOMODIFY:
	return common_setfsgid_nomodify(context);
    default:
	fprintf(stderr, "Test variation [%i] unsupported for %s()\n",
		variation, context->u.syscall.sysname);
	return -1;
    }
}

int test_setfsgid32(struct audit_data *context, int variation, int success)
{
    switch(variation) {
    case TESTSC_MODIFY:
	return common_setfsgid_modify(context);
    case TESTSC_NOMODIFY:
	return common_setfsgid_nomodify(context);
    default:
	fprintf(stderr, "Test variation [%i] unsupported for %s()\n",
		variation, context->u.syscall.sysname);
	return -1;
    }
}
