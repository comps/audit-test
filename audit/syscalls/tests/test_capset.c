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

/* sys/capability.h not available on RHEL4 U2 glibc-2.3.4-2.13 */
extern int capget(cap_user_header_t header, cap_user_data_t data);
extern int capset(cap_user_header_t header, const cap_user_data_t data);

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

    header->version = _LINUX_CAPABILITY_VERSION;
    header->pid = 0;
    rc = capget(header, data);
    if (rc < 0) {
	fprintf(stderr, "Error: capget(): %s\n", strerror(errno));
	goto exit;
    }

    fprintf(stderr, "Effective capabilities: %x\n", data->effective);
    fprintf(stderr, "Inheritable capabilities: %x\n", data->inheritable);
    fprintf(stderr, "Permitted capabilities: %x\n", data->permitted);
     
    if (!success) {
	data->effective = ~data->permitted;
	context_setexperror(context, EPERM);
	fprintf(stderr, "Attempt to set effective capabilities: %x\n", 
		data->effective);
    }

    rc = context_setidentifiers(context);
    if (rc < 0)
	goto exit;

    context_setbegin(context);
    fprintf(stderr, "Attempting %s(%p, %p)\n", 
	    context->u.syscall.sysname, header, data);
    errno = 0;
    exit = capset(header, data);
    context_setend(context);
    context_setresult(context, exit, errno);

exit:
    free(header);
    free(data);
    return rc;
}
