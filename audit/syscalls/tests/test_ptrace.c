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
 *  test_ptrace.c
 *
 *  PURPOSE:
 *  Verify audit of attempts to trace a process.
 *
 *  SYSCALLS:
 *  ptrace()
 *
 *  TESTCASE: successful
 *  Perform a PTRACE_ATTACH to a new child process.
 *
 *  TESTCASE: unsuccessful
 *  Attempt to PTRACE_ATTACH to the init process.
 */

#include "includes.h"
#include <signal.h>
#include <sys/ptrace.h>
#include <sys/wait.h>

int test_ptrace(struct audit_data *context, int variation, int success)
{
    int rc = 0;
    pid_t pid;
    int exit;

    if (success) {
	pid = fork();
	if (pid < 0) {
	    fprintf(stderr, "Error: fork(): %s\n", strerror(errno));
	    rc = -1;
	    goto exit;
	}
	if (pid == 0) {
	    while (1) { ; }
	    _exit(0);
	}
	fprintf(stderr, "Created process: %d\n", pid);
    } else  {
	pid = 1;
	context_setexperror(context, EPERM);
    }

    rc = context_setidentifiers(context);
    if (rc < 0)
	goto exit;

    errno = 0;
    context_setbegin(context);
    fprintf(stderr, "Attempting %s(%x, %x, %p, %p)\n", 
	    context->u.syscall.sysname, PTRACE_ATTACH, pid, NULL, NULL);
    exit = ptrace(PTRACE_ATTACH, pid, NULL, NULL);
    if (exit >= 0)
	wait(NULL);
    context_setend(context);
    context_setresult(context, exit, errno);

    if (exit >= 0 && ptrace(PTRACE_KILL, pid, NULL, NULL) < 0)
	fprintf(stderr, "Error: ptrace(): %s\n", strerror(errno));

exit:
    return rc;
}
