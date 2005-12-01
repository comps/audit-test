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
 *  test_chdir.c
 *
 *  PURPOSE:
 *  Verify audit of attempts to change working directory.
 *
 *  SYSCALLS:
 *  chdir()
 *
 *  TESTCASE: successful
 *  Change working directory to one for which user has access
 *  permissions.
 *
 *  TESTCASE: unsuccessful
 *  Attempt to change working directory to one for which user does not
 *  have access permissions.
 */

#include "includes.h"
#include "syscalls.h"

int test_chdir(struct audit_data *context, int variation, int success)
{
    int rc = 0;
    char *path;
    int exit;

    path = init_tempdir(S_IRWXU, context->euid, context->egid);
    if (!path) {
	rc = -1;
	goto exit;
    }

    if (!success) {
	rc = seteuid_test();
	if (rc < 0)
	    goto exit_path;
	context->experror = -EACCES;
    }

    rc = context_setcwd(context);
    if (rc < 0)
	goto exit_suid;
    context_settobj(context, path);

    rc = context_setidentifiers(context);
    if (rc < 0)
	goto exit_suid;

    errno = 0;
    context_setbegin(context);
    fprintf(stderr, "Attempting chdir(%s)\n", path);
    exit = syscall(context->u.syscall.sysnum, path);
    context_setend(context);
    context_setresult(context, exit, errno);

    errno = 0;
    if ((exit == 0) && (chdir(context_getcwd(context)) < 0))
	fprintf(stderr, "Error: returning to cwd: %s\n", strerror(errno));

exit_suid:
    if (!success && seteuid(0) < 0)
	fprintf(stderr, "Error: seteuid(0): %s\n", strerror(errno));

exit_path:
    destroy_tempdir(path);

exit:
    return rc;
}
