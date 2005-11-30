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
 *  test_unlink.c
 *
 *  PURPOSE:
 *  Verify audit of attempts to remove a name from the filesystem.
 *
 *  SYSCALLS:
 *  unlink()
 *
 *  TESTCASE: successful
 *  Remove a name for which user has appropriate permissions.
 *
 *  TESTCASE: unsuccessful
 *  Attempt to remove a name for which user does not have appropriate
 *  permissions.
 */

#include "includes.h"
#include "syscalls.h"

int test_unlink(struct audit_data *context, int variation, int success)
{
    int rc = 0;
    char *path, *key;
    int exit = -1;

    path = init_tempfile(S_IRWXU, context->euid, context->egid);
    key = audit_add_watch(path);
    if (!path || !key) {
	rc = -1;
	goto exit;
    }

    if (!success) {
	rc = seteuid_test();
	if (rc < 0)
	    goto exit_path;
	context->experror = -EPERM;
    }

    rc = context_setcwd(context);
    if (rc < 0)
	goto exit_suid;
    context_settobj(context, key);

    rc = context_setidentifiers(context);
    if (rc < 0)
	goto exit_suid;

    errno = 0;
    context_setbegin(context);
    fprintf(stderr, "Attempting unlink(%s)\n", path);
    exit = syscall(context->u.syscall.sysnum, path);
    context_setend(context);
    context_setresult(context, exit, errno);

exit_suid:
    if (!success && seteuid(0) < 0)
	fprintf(stderr, "Error: seteuid(0): %s\n", strerror(errno));

exit_path:
    audit_rem_watch(path, key);
    if (exit < 0)
	destroy_tempfile(path);
    else
	free(path);
    free(key);

exit:
    return rc;
}
