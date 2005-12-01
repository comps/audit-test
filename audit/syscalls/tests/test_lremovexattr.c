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
 *  test_lremovexattr.c
 *
 *  PURPOSE:
 *  Verify audit of attempts to remove extended attributes.
 *
 *  SYSCALLS:
 *  lremovexattr()
 *
 *  TESTCASE: file successful
 *  Remove an extended attribute for a file for which user has
 *  appropriate permissions.
 *
 *  TESTCASE: file unsuccessful
 *  Attempt to remove an extended attribute for a file for which
 *  user does not have appropriate permissions.
 *
 *  TESTCASE: symlink successful
 *  Remove an extended attribute for a symlink for which user has
 *  appropriate permissions.
 *
 *  TESTCASE: symlink unsuccessful
 *  Attempt to remove an extended attribute for a symlink for which
 *  user does not have appropriate permissions.
 */

#include "includes.h"
#include "syscalls.h"
#include <attr/xattr.h>

static int test_lremovexattr_file(struct audit_data *context, int success)
{
    int rc = 0;
    char *path, *key;
    char *aname = TEST_FILE_XATTR_NAME;
    char *avalue = TEST_FILE_XATTR_VALUE;
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

    errno = 0;
    rc = setxattr(path, aname, avalue, sizeof(avalue), XATTR_CREATE);
    if (rc < 0) {
	fprintf(stderr, "Error: creating extended attribute %s: %s\n",
		aname, strerror(errno));
	goto exit;
    }
    fprintf(stderr, "Created extended attribute %s for %s\n", aname, path);

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
    fprintf(stderr, "Attempting removexattr(%s, %s)\n", path, aname);
    exit = syscall(context->u.syscall.sysnum, path, aname);
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

static int test_lremovexattr_symlink(struct audit_data *context, int success)
{
    int rc = 0;
    char *target, *path;
    char *aname = TEST_SYM_XATTR_NAME;
    char *avalue = TEST_SYM_XATTR_VALUE;
    int exit;

    target = init_tempfile(S_IRWXU, context->euid, context->egid);
    if (!target) {
	rc = -1;
	goto exit;
    }

    path = init_tempsym(target, context->euid, context->egid);
    if (!path) {
	destroy_tempfile(target);
	rc = -1;
	goto exit;
    }

    errno = 0;
    rc = lsetxattr(path, aname, avalue, sizeof(avalue), XATTR_CREATE);
    if (rc < 0) {
	fprintf(stderr, "Error: creating extended attribute %s: %s\n",
		aname, strerror(errno));
	goto exit;
    }
    fprintf(stderr, "Created extended attribute %s for %s\n", aname, path);

    if (!success) {
	rc = seteuid_test();
	if (rc < 0)
	    goto exit_path;
	context->experror = -EACCES;
    }

    rc = context_setcwd(context);
    if (rc < 0)
	goto exit_suid;
    context_setsym(context, path);

    rc = context_setidentifiers(context);
    if (rc < 0)
	goto exit_suid;

    errno = 0;
    context_setbegin(context);
    fprintf(stderr, "Attempting lremovexattr(%s, %s)\n", path, aname);
    exit = syscall(context->u.syscall.sysnum, path, aname);
    context_setend(context);
    context_setresult(context, exit, errno);

exit_suid:
    if (!success && seteuid(0) < 0)
	fprintf(stderr, "Error: seteuid(0): %s\n", strerror(errno));

exit_path:
    destroy_tempsym(path);
    destroy_tempfile(target);

exit:
    return rc;
}

int test_lremovexattr(struct audit_data *context, int variation, int success)
{
    switch(variation) {
    case SYSCALL_FILE:
	return test_lremovexattr_file(context, success);
    case SYSCALL_SYMLINK:
	return test_lremovexattr_symlink(context, success);
    default:
	fprintf(stderr, "Test variation [%i] unsupported for %s()\n",
		variation, context->u.syscall.sysname);
	return -1;
    }
}
