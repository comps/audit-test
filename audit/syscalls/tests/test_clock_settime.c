/*  (c) Copyright Hewlett-Packard Development Company, L.P., 2005
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
 *  FILE:
 *  test_clock_settime.c
 *
 *  PURPOSE:
 *  Verify audit of attempts to change system time.
 *
 *  SYSCALLS:
 *  clock_settime()
 *
 *  TESTCASE: successful
 *  Set system time as root user.
 *
 *  TESTCASE: unsuccessful
 *  Attempt to set system time as test user.
 */

#include "includes.h"
#include "syscalls.h"
#include <time.h>

int test_clock_settime(struct audit_data *context)
{
    int rc = 0;
    int success = context->success; /* save intended result */
    struct timespec tspec;
    int exit;

    /* Get current time */
    rc = syscall(__NR_clock_gettime, CLOCK_REALTIME, &tspec);
    if (rc < 0) {
	fprintf(stderr, "Error: getting current time: %s\n", strerror(errno));
	goto exit;
    }

    /* To produce failure, attempt to set system time as unprivileged user */
    if (!success) {
	rc = seteuid_test();
	if (rc < 0)
	    goto exit;
	context->experror = -EPERM;
    }

    rc = context_setidentifiers(context);
    if (rc < 0)
	goto exit;

    errno = 0;
    context_setbegin(context);
    exit = -1;
    exit = syscall(context->u.syscall.sysnum, CLOCK_REALTIME, &tspec);
    context_setend(context);

    if (exit < 0) {
	context->success = 0;
	context->u.syscall.exit = context->error = -errno;
    } else {
	context->success = 1;
	context->u.syscall.exit = exit;
    }

exit:
    if (!success)
	seteuid(0); /* clean up from failure case */
    return rc;
}
