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
 *  test_fork.c
 *
 *  PURPOSE:
 *  Verify audit of attempts to create a child process.
 *
 *  SYSCALLS:
 *  fork(), vfork()
 *
 *  TESTCASE: successful
 *  Create a child process as root user.
 *
 *  TESTCASE: unsuccessful
 *  With RLIMIT_NPROC set to 1, attempt to create a child process as
 *  the test user.
 */

#include "includes.h"
#include "syscalls.h"
#include <sys/resource.h>
#include <sched.h>

#define TEST_FORK  0
#define TEST_VFORK 1

int common_fork(struct audit_data *context, int op)
{
    int rc = 0;
    int success = context->success; /* save intended result */
    struct rlimit slimit, limit;
    pid_t pid = -1;

    if (!success) {
	rc = setresuid_test();
	if (rc < 0)
	    goto exit;

	rc = getrlimit(RLIMIT_NPROC, &slimit);
	if (rc < 0) {
	    fprintf(stderr, "Error: unable to get RLIMIT_NPROC limit: %s\n",
		    strerror(errno));
	    goto exit;
	}

	limit.rlim_cur = 1;
	limit.rlim_max = slimit.rlim_max;
	rc = setrlimit(RLIMIT_NPROC, &limit);
	if (rc < 0) {
	    fprintf(stderr, "Error: unable to set RLIMIT_NPROC limit: %s\n",
		    strerror(errno));
	    goto exit;
	}
	context->experror = EAGAIN;
    }

    rc = context_setidentifiers(context);
    if (rc < 0)
	goto exit;

    errno = 0;
    context_setbegin(context);

    /* Using the vfork() library function is significant, as with
     * syscall(), control doesn't return to the calling function
     * (parent) in the success case. */
    if (op == TEST_FORK)
	pid = fork();
    else if (op == TEST_VFORK)
	pid = vfork();
    if (pid == 0)
	_exit(0);
    context_setend(context);

    if (pid < 0) {
	context->success = 0;
	context->error = context->u.syscall.exit = errno;
    } else {
	context->success = 1;
	context->u.syscall.exit = pid;
    }

exit:
    if (!success) {
	if (setrlimit(RLIMIT_NPROC, &slimit) < 0)
	    fprintf(stderr, "Error: unable to reset RLIMIT_NPROC limit: %s\n",
		    strerror(errno));
	setresuid(0, 0, 0); 
    }
    return rc;
}

int test_fork(struct audit_data *context)
{
    return common_fork(context, TEST_FORK);
}

int test_vfork(struct audit_data *context)
{
    return common_fork(context, TEST_VFORK);
}
