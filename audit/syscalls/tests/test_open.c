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
 *  test_open.c
 *
 *  PURPOSE:
 *  Verify audit of attempts to open files.
 *
 *  SYSCALLS:
 *  open()
 *
 *  TESTCASE: successful
 *  Open a file for which user has access permissions.
 *
 *  TESTCASE: unsuccessful
 *  Attempt to open a file for which user does not have access
 *  permissions.
 */

#include "includes.h"
#include "syscalls.h"

int test_open(struct audit_data *context, int variation, int success)
{
    int rc = 0;
    char *path;
    int flags = O_RDONLY;
    int fd;

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
    fprintf(stderr, "Attempting %s(%s, %x)\n", 
	    context->u.syscall.sysname, path, flags);
    errno = 0;
    fd = syscall(context->u.syscall.sysnum, path, flags);
    context_setend(context);
    context_setresult(context, fd, errno);

    errno = 0;
    if ((fd != -1) && (close(fd) < 0))
	fprintf(stderr, "Error: closing file: %s\n", strerror(errno));

exit_suid:
    errno = 0;
    if (!success && (seteuid(0) < 0))
	fprintf(stderr, "Error: seteuid(): %s\n", strerror(errno));

exit_path:
    destroy_tempfile(path);

exit:
    return rc;
}
