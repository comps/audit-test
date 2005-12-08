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
 *  test_fremovexattr.c
 *
 *  PURPOSE:
 *  Verify audit of attempts to remove extended attributes via a file
 *  descriptor.
 *
 *  SYSCALLS:
 *  fremovexattr()
 *
 *  TESTCASE: successful
 *  Remove an extended attribute for a file for which user has
 *  appropriate permissions.
 *
 *  TESTCASE: unsuccessful
 *  Attempt to remove an extended attribute for a file for which
 *  user does not have appropriate permissions.
 */

#include "includes.h"
#include "syscalls.h"
#include <attr/xattr.h>

int test_fremovexattr(struct audit_data *context, int variation, int success)
{
    int rc = 0;
    char *path, *key;
    int fd;
    char *aname = TEST_FILE_XATTR_NAME;
    char *avalue = TEST_FILE_XATTR_VALUE;
    int exit;

    path = init_tempfile(S_IRWXU, context->euid, context->egid,
			 context->u.syscall.sysname);
    if (!path) {
	rc = -1;
	goto exit;
    }

    key = audit_add_watch(path);
    if (!key) {
	destroy_tempfile(path);
	rc = -1;
	goto exit;
    }

    errno = 0;
    rc = setxattr(path, aname, avalue, sizeof(avalue), XATTR_CREATE);
    if (rc < 0) {
	fprintf(stderr, "Error: creating extended attribute %s: %s\n",
		aname, strerror(errno));
	goto exit_path;
    }
    fprintf(stderr, "Created extended attribute %s for %s\n", aname, path);

    rc = fd = open(path, O_RDWR);
    if (rc < 0) {
	fprintf(stderr, "Error: Could not open %s: %s\n", path,
		strerror(errno));
	goto exit_path;
    }
    fprintf(stderr, "Opened file: %s fd: %i\n", path, fd);

    if (!success) {
	rc = seteuid_test();
	if (rc < 0)
	    goto exit_path;
	context_setexperror(context, EACCES);
    }

    context_setwatch(context, key);
    rc = context_setidentifiers(context);
    if (rc < 0)
	goto exit_suid;

    context_setbegin(context);
    fprintf(stderr, "Attempting %s(%x, %s)\n", 
	    context->u.syscall.sysname, fd, aname);
    errno = 0;
    exit = syscall(context->u.syscall.sysnum, fd, aname);
    context_setend(context);
    context_setresult(context, exit, errno);

exit_suid:
    errno = 0;
    if (!success && (seteuid(0) < 0))
	fprintf(stderr, "Error: seteuid(): %s\n", strerror(errno));

exit_path:
    audit_rem_watch(path, key);
    destroy_tempfile(path);
    free(key);

exit:
    return rc;
}
