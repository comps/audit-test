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
 *  test_mknod.c
 *
 *  PURPOSE:
 *  Verify audit of attempts to create special files.
 *
 *  SYSCALLS:
 *  mknod()
 *
 *  TESTCASE: successful
 *  Create a special file in a directory for which user has
 *  appropriate permissions.
 *
 *  TESTCASE: unsuccessful
 *  Attempt to create a special file in a directory for which user
 *  does not have appropriate permissions.
 */

#include "includes.h"
#include "syscalls.h"

int test_mknod(struct audit_data *context, int variation, int success)
{
    int rc = 0;
    char *tmp, *dir, *path;
    char *newname = "/blkdev";
    mode_t mode = S_IRWXU|S_IRWXO;
    dev_t dev = S_IFBLK;
    int exit;

    tmp = init_tempdir(S_IRWXU, context->euid, context->egid);
    if (!tmp) {
	rc = -1;
	goto exit;
    }
    dir = strdup(tmp);

    errno = 0;
    path = realloc(tmp, strlen(tmp) + strlen(newname) + 1);
    if (!path) {
	fprintf(stderr, "Error: initializing path: realloc(): %s\n",
		strerror(errno));
	destroy_tempdir(dir);
	free(tmp);
	rc = -1;
	goto exit;
    }

    if (strcat(path, newname) == NULL) {
	fprintf(stderr, "Error: initializing path: strcat(): %s\n",
		strerror(errno));
	rc = -1;
	goto exit_path;
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
    if (success) {
	context_settype(context, AUDIT_MSG_PATH_DIR);
	context_settdir(context, dir);
	context_settobj(context, path);
    } else {
	context_settype(context, AUDIT_MSG_PATH);
	context_settobj(context, path);
    }

    rc = context_setidentifiers(context);
    if (rc < 0)
	goto exit_suid;

    context_setbegin(context);
    fprintf(stderr, "Attempting %s(%s, %x, S_IFBLK)\n", 
	    context->u.syscall.sysname, path, mode);
    errno = 0;
    exit = syscall(context->u.syscall.sysnum, path, mode, dev);
    context_setend(context);
    context_setresult(context, exit, errno);

    errno = 0;
    if ((exit == 0) && (unlink(path) < 0))
	fprintf(stderr, "Error: removing file: %s\n", strerror(errno));

exit_suid:
    errno = 0;
    if (!success && (seteuid(0) < 0))
	fprintf(stderr, "Error: seteuid(): %s\n", strerror(errno));

exit_path:
    destroy_tempdir(dir);
    free(path);

exit:
    return rc;
}
