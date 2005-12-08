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
 *  test_clone.c
 *
 *  PURPOSE:
 *  Verify audit of attempts to create child processes.
 *
 *  SYSCALLS:
 *  clone(), clone2
 *
 *  TESTCASE: successful
 *  Create a child process with CLONE_NEWNS as root user.
 *
 *  TESTCASE: unsuccessful
 *  Attempt to create a child process with CLONE_NEWNS as test user.
 */

#include "includes.h"
#include "syscalls.h"
#include <sched.h>

#define CHILD_STACK_MEM 65536

static int common_clone(struct audit_data *context, int success)
{
    int rc = 0;
    int flags = CLONE_NEWNS|CLONE_VFORK; /* wait for child exit */
    char *cstack;
    pid_t pid;

    /* Allocate memory for child stack */
    errno = 0;
    cstack = malloc(CHILD_STACK_MEM); 
    if (!cstack) {
	fprintf(stderr, "Error: malloc(): %s\n", strerror(errno));
	goto exit;
    }
    fprintf(stderr, "Allocated %d for child stack\n", CHILD_STACK_MEM);

    /* To produce failure, attempt to use CLONE_NEWNS as unprivileged user */
    if (!success) {
	rc = seteuid_test();
	if (rc < 0)
	    goto exit_free;
	context_setexperror(context, EPERM);
    }

    rc = context_setidentifiers(context);
    if (rc < 0)
	goto exit_free;

    errno = 0;
    context_setbegin(context);
    fprintf(stderr, "Create child at %p\n", cstack);
    pid = syscall(context->u.syscall.sysnum, flags, cstack);

    if (pid == 0) /* child */
	_exit(0);

    context_setend(context);

    if (pid < 0) {
	context->success = 0;
	context->u.syscall.exit = context->error = -errno;
    } else {
	context->success = 1;
	context->u.syscall.exit = pid;
    }

exit_free:
    free(cstack);

exit:
    if (!success)
	seteuid(0); /* clean up from failure case */
    return rc;
}

int test_clone(struct audit_data *context, int variation, int success)
{
    return common_clone(context, success);
}

int test_clone2(struct audit_data *context, int variation, int success)
{
    return common_clone(context, success);
}
