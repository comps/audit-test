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
 *  test_chown.c
 *
 *  PURPOSE:
 *  Verify audit of attempts to change file owner and group.
 *
 *  SYSCALLS:
 *  chown(), chown32()
 *
 *  TESTCASE: successful
 *  Change a file's owner and group as the superuser.
 *
 *  TESTCASE: unsuccessful
 *  Attempt to change a file's owner and group as regular user.
 */

#include "includes.h"
#include "syscalls.h"

static int common_chown(struct audit_data *context, int success)
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
    fprintf(stderr, "Attempting %s(%s, %x, %x)\n", 
	    context->u.syscall.sysname, path, owner, group);
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

int test_chown(struct audit_data *context, int variation, int success)
{
    return common_chown(context, success);
}

int test_chown32(struct audit_data *context, int variation, int success)
{
    return common_chown(context, success);
}
