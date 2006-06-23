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
 *  test_rmdir.c
 *
 *  PURPOSE:
 *  Verify audit of attempts to remove a directory.
 *
 *  SYSCALLS:
 *  rmdir()
 *
 *  TESTCASE: successful
 *  Remove a directory for which user has appropriate permissions.
 *
 *  TESTCASE: unsuccessful
 *  Attempt to remove a directory for which user does not
 *  have appropriate permissions.
 */

#include "includes.h"
#include "syscalls.h"
#include <libgen.h>

int test_rmdir(struct audit_data *context, int variation, int success)
{
    int rc = 0;
    char *path, *dir;
    int exit = -1;

    path = init_tempdir(S_IRWXU, context->euid, context->egid);
    if (!path) {
	rc = -1;
	goto exit;
    }
    dir = strdup(path);

    if (!success) {
	rc = seteuid_test();
	if (rc < 0)
	    goto exit_path;
	context_setexperror(context, EPERM);
    }

    rc = context_setcwd(context);
    if (rc < 0)
	goto exit_suid;
    context_settype(context, AUDIT_MSG_PATH_DIR);
    context_settdir(context, dirname(dir));
    context_settobj(context, path);

    rc = context_setidentifiers(context);
    if (rc < 0)
	goto exit_suid;

    context_setbegin(context);
    fprintf(stderr, "Attempting %s(%s)\n", 
	    context->u.syscall.sysname, path);
    errno = 0;
    exit = syscall(context->u.syscall.sysnum, path);
    context_setend(context);
    context_setresult(context, exit, errno);

exit_suid:
    errno = 0;
    if (!success && (seteuid(0) < 0))
	fprintf(stderr, "Error: seteuid(): %s\n", strerror(errno));

exit_path:
    errno = 0;
    if (exit < 0)
	destroy_tempdir(path);
    else
	free(path);
    free(dir);

exit:
    return rc;
}
