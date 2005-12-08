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
 *  test_setgroups.c
 *
 *  PURPOSE:
 *  Verify audit of attempts to set a process's list of supplementary
 *  group IDs.
 *
 *  SYSCALLS:
 *  setgroups(), setgroups32()
 *
 *  TESTCASE: successful
 *  As root, set the list of supplementary group IDs to the values in
 *  the list returned by getgroups().
 *
 *  TESTCASE: unsuccessful
 *  As test user, attempt to set the list of supplementary group IDs
 *  to the list obtained as root user.
 */

#include "includes.h"
#include "syscalls.h"
#include <grp.h>

/* expect no more than 20 supplementary groups */
#define SUPP_GROUPS 20

static int common_setgroups(struct audit_data *context, int success)
{
    int rc = 0;
    gid_t list[SUPP_GROUPS] = { 0 };
    size_t size = SUPP_GROUPS;
    int i, exit;

    errno = 0;
    rc = getgroups(size, list);
    if (rc < 0) {
	fprintf(stderr, "Error:  getgroups(): %s\n", strerror(errno));
	goto exit;
    }
    fprintf(stderr, "Process groups: ");
    for (i = 0; i < size; i++)
	fprintf(stderr, "%i ", list[i]);
    fprintf(stderr, "\n");

    /* To produce failure, become test user and attempt to set groups
     * to list obtained as root user */
    if (!success) {
	rc = seteuid_test();
	if (rc < 0)
	    goto exit;
	context_setexperror(context, EPERM);
    }

    rc = context_setidentifiers(context);
    if (rc < 0)
	goto exit;

    errno = 0;
    context_setbegin(context);
    exit = syscall(context->u.syscall.sysnum, size, &list);
    context_setend(context);
    context_setresult(context, exit, errno);

exit:
    if (!success)
	seteuid(0); /* clean up from failure case */
    return rc;
}

int test_setgroups(struct audit_data *context, int variation, int success)
{
    return common_setgroups(context, success);
}

int test_setgroups32(struct audit_data *context, int variation, int success)
{
    return common_setgroups(context, success);
}
