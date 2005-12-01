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
 *  test_setxattr.c
 *
 *  PURPOSE:
 *  Verify audit of attempts to set extended attribute values.
 *
 *  SYSCALLS:
 *  setxattr()
 *
 *  TESTCASE: successful
 *  Set an extended attribute value for a file for which user has
 *  appropriate permissions.
 *
 *  TESTCASE: unsuccessful
 *  Attempt to set an extended attribute value for a file for which
 *  user does not have appropriate permissions.
 */

#include "includes.h"
#include "syscalls.h"
#include <attr/xattr.h>

int test_setxattr(struct audit_data *context, int variation, int success)
{
    int rc = 0;
    char *path, *key;
    char *aname = TEST_FILE_XATTR_NAME;
    char *avalue = TEST_FILE_XATTR_VALUE;
    int flags = XATTR_CREATE;
    int exit;

    path = init_tempfile(S_IRWXU, context->euid, context->egid);
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

    if (!success) {
	rc = seteuid_test();
	if (rc < 0)
	    goto exit_path;
	context->experror = -EACCES;
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
    fprintf(stderr, "Attempting setxattr(%s, %s, %s, %zi, %i)\n", 
	    path, aname, avalue, sizeof(avalue), flags);
    exit = syscall(context->u.syscall.sysnum, path, aname, avalue,
		   sizeof(avalue), flags);
    context_setend(context);
    context_setresult(context, exit, errno);

exit_suid:
    if (!success && seteuid(0) < 0)
	fprintf(stderr, "Error: seteuid(0): %s\n", strerror(errno));

exit_path:
    audit_rem_watch(path, key);
    destroy_tempfile(path);
    free(key);

exit:
    return rc;
}
