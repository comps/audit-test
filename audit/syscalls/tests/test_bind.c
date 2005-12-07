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
 *  test_bind.c
 *
 *  PURPOSE:
 *  Verify audit of attempts to bind a name to a socket.
 *
 *  SYSCALLS:
 *  bind()
 *
 *  TESTCASE: successful
 *  Bind to a privileged port as the super user.
 *
 *  TESTCASE: unsuccessful
 *  Attempt to bind to a privileged port as a non-privileged user.
 */

#include "includes.h"
#include "syscalls.h"
#include <sys/socket.h>
#include <netinet/in.h>

int test_bind(struct audit_data *context, int variation, int success)
{
    int rc = 0;
    int sockfd;
    struct sockaddr_in my_addr;
    socklen_t addrlen = sizeof(my_addr);
    int port = 24; /* site-dependent mail handling, unused */
    int exit;

    rc = sockfd = socket(PF_INET, SOCK_STREAM, 0);
    if (rc < 0) {
	fprintf(stderr, "Error: creating socket: %s\n", strerror(errno));
	goto exit;
    }

    memset(&my_addr, 0, addrlen);
    my_addr.sin_family = AF_INET;
    my_addr.sin_port = htons(port);
    my_addr.sin_addr.s_addr = INADDR_NONE;
    context_setsockaddr(context, (unsigned char *)&my_addr, addrlen);

    if (!success) {
	rc = seteuid_test();
	if (rc < 0)
	    goto exit_sock;
	context->experror = -EACCES;
    }

    rc = context_setidentifiers(context);
    if (rc < 0)
	goto exit_suid;

    errno = 0;
    context_setbegin(context);
    fprintf(stderr, "Attempting bind(%d, %x, %d)\n", 
	    sockfd, my_addr.sin_addr.s_addr, addrlen);
    exit = syscall(context->u.syscall.sysnum, sockfd, 
		   (struct sockaddr *)&my_addr, addrlen);
    context_setend(context);
    context_setresult(context, exit, errno);

exit_suid:
    if (!success && seteuid(0) < 0)
	fprintf(stderr, "Error: seteuid(0): %s\n", strerror(errno));

exit_sock:
    if (close(sockfd) < 0)
	fprintf(stderr, "Error: closing sockfd: %s\n", strerror(errno));

exit:
    return rc;
}
