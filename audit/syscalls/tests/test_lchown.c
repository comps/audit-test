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
 *  test_lchown.c
 *
 *  PURPOSE:
 *  Verify audit of attempts to change file owner and group.
 *
 *  SYSCALLS:
 *  lchown(), lchown32()
 *
 *  TESTCASE: file successful
 *  Change a file's owner and group as the superuser.
 *
 *  TESTCASE: file unsuccessful
 *  Attempt to change a file's owner and group as regular user.
 *
 *  TESTCASE: symlink successful
 *  Change a symlink's owner and group as the superuser.
 *
 *  TESTCASE: symlink unsuccessful
 *  Attempt to change a symlink's owner and group as regular user.
 */

#include "includes.h"
#include "syscalls.h"

static int common_lchown_file(struct audit_data *context, int success)
{
    int rc = 0;
    char *path;
    uid_t owner;
    gid_t group;
    int exit;

    owner = gettestuid();
    group = gettestgid();
    if ((owner < 0) || (group < 0)) {
	rc = -1;
	goto exit;
    }

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
	context_setexperror(context, EPERM);
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
    fprintf(stderr, "Attempting lchown(%s, %i, %i)\n", path, owner, group);
    exit = syscall(context->u.syscall.sysnum, path, owner, group);
    context_setend(context);
    context_setresult(context, exit, errno);

exit_suid:
    if (!success && seteuid(0) < 0)
	fprintf(stderr, "Error: seteuid(0): %s\n", strerror(errno));

exit_path:
    destroy_tempfile(path);

exit:
    return rc;
}

static int common_lchown_symlink(struct audit_data *context, int success)
{
    int rc = 0;
    char *target, *path;
    uid_t owner;
    gid_t group;
    int exit;

    owner = gettestuid();
    group = gettestgid();
    if ((owner < 0) || (group < 0)) {
	rc = -1;
	goto exit;
    }

    target = init_tempfile(S_IRWXU, context->euid, context->egid,
			   context->u.syscall.sysname);
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

    if (!success) {
	rc = seteuid_test();
	if (rc < 0)
	    goto exit_path;
	context_setexperror(context, EPERM);
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
    fprintf(stderr, "Attempting lchown(%s, %i, %i)\n", path, owner, group);
    exit = syscall(context->u.syscall.sysnum, path, owner, group);
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

int test_lchown(struct audit_data *context, int variation, int success)
{
    switch(variation) {
    case SYSCALL_FILE:
	return common_lchown_file(context, success);
    case SYSCALL_SYMLINK:
	return common_lchown_symlink(context, success);
    default:
        fprintf(stderr, "Test variation [%i] unsupported for %s()\n",
                variation, context->u.syscall.sysname);
        return -1;
    }
}

int test_lchown32(struct audit_data *context, int variation, int success)
{
    switch(variation) {
    case SYSCALL_FILE:
	return common_lchown_file(context, success);
    case SYSCALL_SYMLINK:
	return common_lchown_symlink(context, success);
    default:
        fprintf(stderr, "Test variation [%i] unsupported for %s()\n",
                variation, context->u.syscall.sysname);
        return -1;
    }
}


