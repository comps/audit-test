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
 *  test_truncate.c
 *
 *  PURPOSE:
 *  Verify audit of attempts to truncate a file to a specified length.
 *
 *  SYSCALLS:
 *  truncate(), truncate64()
 *
 *  TESTCASE: successful
 *  Trucate a file for which user has appropriate permissions.
 *
 *  TESTCASE: unsuccessful
 *  Attempt to truncate a file for which user does not have
 *  appropriate permissions.
 */

#include "includes.h"
#include "syscalls.h"

#define TEST_TRUNCATE	0
#define TEST_TRUNCATE64	1

int common_truncate(struct audit_data *context, int op, int success)
{
    int rc = 0;
    char *path;
    int exit = -1;

    path = init_tempfile(S_IRWXU, context->euid, context->egid, 
			 context->u.syscall.sysname);
    if (!path) {
	rc = -1;
	goto exit;
    }

    if (!success) {
	rc = seteuid_test();
	if (rc < 0)
	    goto exit_path;
	context_setexperror(context, EACCES);
    }

    rc = context_setcwd(context);
    if (rc < 0)
	goto exit_suid;
    context_settobj(context, path);

    rc = context_setidentifiers(context);
    if (rc < 0)
	goto exit_suid;

    context_setbegin(context);
    if (op == TEST_TRUNCATE) {
	fprintf(stderr, "Attempting %s(%s, %x)\n", 
		context->u.syscall.sysname, path, 0);
	errno = 0;
	exit = syscall(context->u.syscall.sysnum, path, 0);
    } else if (op == TEST_TRUNCATE64) {
	fprintf(stderr, "Attempting %s(%s, %x, %x)\n", 
		context->u.syscall.sysname, path, 0, 0);
	errno = 0;
	exit = syscall(context->u.syscall.sysnum, path, 0, 0);
    }
    context_setend(context);
    context_setresult(context, exit, errno);

exit_suid:
    errno = 0;
    if (!success && (seteuid(0) < 0))
	fprintf(stderr, "Error: seteuid(): %s\n", strerror(errno));

exit_path:
    destroy_tempfile(path);

exit:
    return rc;
}

int test_truncate(struct audit_data *context, int variation, int success)
{
    return common_truncate(context, TEST_TRUNCATE, success);
}

int test_truncate64(struct audit_data *context, int variation, int success)
{
    return common_truncate(context, TEST_TRUNCATE64, success);
}
