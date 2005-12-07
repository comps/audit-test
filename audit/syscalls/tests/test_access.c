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
 *  test_access.c
 *
 *  PURPOSE:
 *  Verify audit of attempts to check file access permissions.
 *
 *  SYSCALLS:
 *  access()
 *
 *  TESTCASE: successful
 *  Check read access permissions on a file with read permissions.
 *
 *  TESTCASE: unsuccessful
 *  Attempt to check write access permissions on a file with read-only
 *  permissions.
 */

#include "includes.h"
#include "syscalls.h"

int test_access(struct audit_data *context, int variation, int success)
{
    int rc = 0;
    char *path;
    int mode;
    int fd;

    path = init_tempfile(S_IRUSR|S_IROTH, context->euid, context->egid,
			 context->u.syscall.sysname);
    if (!path) {
	rc = -1;
	goto exit;
    }

    if (success)
	mode = R_OK;
    else {
	mode = W_OK;
	rc = setresuid_test();
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
    fprintf(stderr, "Attempting access(%s, %d)\n", path, mode);
    fd = syscall(context->u.syscall.sysnum, path, mode);
    context_setend(context);
    context_setresult(context, fd, errno);

    errno = 0;
    if ((fd != -1) && (close(fd) < 0))
	fprintf(stderr, "Error: closing file: %s\n", strerror(errno));

exit_suid:
    if (!success && setresuid(0, 0, 0) < 0)
	fprintf(stderr, "Error: seteuid(0): %s\n", strerror(errno));

exit_path:
    destroy_tempfile(path);

exit:
    return rc;
}
