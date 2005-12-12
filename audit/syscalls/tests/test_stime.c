/*  (c) Copyright Hewlett-Packard Development Company, L.P., 2005
 *  Written by Amy Griffis <amy.griffis@hp.com>
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
 *  test_stime.c
 *
 *  PURPOSE:
 *  Verify audit of attempts to change system time.
 *
 *  SYSCALLS:
 *  stime()
 *
 *  TESTCASE: successful
 *  Set system time as root user.
 *
 *  TESTCASE: unsuccessful
 *  Attempt to set system time as test user.
 */

#include "includes.h"
#include "syscalls.h"
#include <sys/time.h>

int test_stime(struct audit_data *context, int variation, int success)
{
    int rc = 0;
    struct timeval tv;
    struct timezone tz;
    int exit;

    rc = gettimeofday(&tv, &tz);
    if (rc < 0) {
	fprintf(stderr, "Error: getting current time: %s\n", strerror(errno));
	goto exit;
    }

    if (!success) {
	rc = seteuid_test();
	if (rc < 0)
	    goto exit;
	context_setexperror(context, EPERM);
    }

    rc = context_setidentifiers(context);
    if (rc < 0)
	goto exit_suid;

    context_setbegin(context);
    fprintf(stderr, "Attempting %s(%p)\n",
	    context->u.syscall.sysname, &tv.tv_sec);
    errno = 0;
    exit = syscall(context->u.syscall.sysnum, &tv.tv_sec);
    context_setend(context);
    context_setresult(context, exit, errno);

exit_suid:
    if (!success && (seteuid(0) < 0))
	fprintf(stderr, "Error: seteuid(): %s\n", strerror(errno));

exit:
    return rc;
}
