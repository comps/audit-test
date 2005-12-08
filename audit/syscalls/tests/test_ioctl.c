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
 *  test_ioctl.c
 *
 *  PURPOSE:
 *  Verify audit of device control operations.
 *
 *  SYSCALLS:
 *  ioctl()
 *
 *  TESTCASE: successful
 *  Get tty attributes of DEFAULT_DEVICE_FILE.
 *
 *  TESTCASE: unsuccessful
 *  Attempt to get tty attributes for invalid descriptor.
 */

#include "includes.h"
#include "syscalls.h"
#include <termio.h>
#include <sys/ioctl.h>

#define DEFAULT_DEVICE_FILE "/dev/tty"

int test_ioctl(struct audit_data *context, int variation, int success)
{
    int rc = 0;
    int fd = -1;
    struct termio tio;
    int exit;

    if (success) {
	fd = open(DEFAULT_DEVICE_FILE, O_RDWR, 0777);
	if (fd < 0) {
	    rc = -1;
	    fprintf(stderr, "Error: Cannot open tty device %s\n", 
		    DEFAULT_DEVICE_FILE);
	    goto exit;
	}
    } else
	context_setexperror(context, EBADF);

    rc = context_setidentifiers(context);
    if (rc < 0)
	goto exit;

    fprintf(stderr, "Attempt to call %s(%d, %x, %p)\n", 
	    context->u.syscall.sysname, fd, TCGETA, &tio);

    errno = 0;
    context_setbegin(context);
    fprintf(stderr, "Attempting %s(%x, %x, %p)\n",
	    context->u.syscall.sysname, fd, TCGETA, &tio);
    exit = syscall(context->u.syscall.sysnum, fd, TCGETA, &tio);
    context_setend(context);
    context_setresult(context, exit, errno);

exit:
    close(fd);
    return rc;
}
