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
 *  test_capset.c
 *
 *  PURPOSE:
 *  Verify audit of attempts to set process capabilities.
 *
 *  SYSCALLS:
 *  capset()
 *
 *  TESTCASE: successful
 *  Set capabilities as obtained from capget().
 *
 *  TESTCASE: unsuccessful
 *  Attempt to set a capability in the effective set that is not in
 *  the permitted set.
 */

#include "includes.h"
#include "syscalls.h"
#include <linux/capability.h>
#if !defined(__ia64__)
#undef _POSIX_SOURCE
#include <sys/capability.h>
#define _POSIX_SOURCE
#else
extern int capget(cap_user_header_t header, cap_user_data_t data);
extern int capset(cap_user_header_t header, const cap_user_data_t data);
#endif

int test_capset(struct audit_data *context, int variation, int success)
{
    int rc = 0;
    cap_user_header_t header;
    cap_user_data_t data;
    int exit;

    errno = 0;
    header = (cap_user_header_t)malloc(sizeof(cap_user_header_t));
    data = (cap_user_data_t)malloc(sizeof(cap_user_data_t));
    if (!header || !data) {
	fprintf(stderr, "Error: malloc(): %s\n", strerror(errno));
	return -1;
    }

    /* get current process's capabilities */
    header->version = _LINUX_CAPABILITY_VERSION;
    header->pid = 0;

    errno = 0;
    rc = capget(header, data);
    if (rc < 0) {
	fprintf(stderr, "Error: capget(): %s\n", strerror(errno));
	goto exit;
    }

    fprintf(stderr, "Effective capabilities: %x\n", data->effective);
    fprintf(stderr, "Inheritable capabilities: %x\n", data->inheritable);
    fprintf(stderr, "Permitted capabilities: %x\n", data->permitted);
     
    /* To produce failure, attempt to set a capability in the
     * effective set that is not in the permitted set. */
    if (!success) {
	data->effective = ~data->permitted;
	context->experror = -EPERM;
	fprintf(stderr, "Attempt to set effective capabilities: %x\n", 
		data->effective);
    }

    rc = context_setidentifiers(context);
    if (rc < 0)
	goto exit;

    errno = 0;
    context_setbegin(context);
    exit = capset(header, data);
    context_setend(context);

    if (exit < 0) {
	context->success = 0;
	context->u.syscall.exit = context->error = -errno;
    } else {
	context->success = 1;
	context->u.syscall.exit = exit;
    }

exit:
    free(header);
    free(data);
    return rc;
}
