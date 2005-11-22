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
 *  test_execve.c
 *
 *  PURPOSE:
 *  Verify audit of attempts to execute a program.
 *
 *  SYSCALLS:
 *  execve()
 *
 *  TESTCASE: successful
 *  Execute auditctl program as root user.
 *
 *  TESTCASE: unsuccessful
 *  Attempt to execute auditctl program as test user.
 */

#include "includes.h"
#include "syscalls.h"
#include <sys/wait.h>

char *argv[] = { "auditctl", "-v", NULL };

int test_execve(struct audit_data *context)
{
    int rc = 0;
    int success = context->success; /* save intended result */
    pid_t pid;
    int status, exit;

    /* To produce failure, attempt to execute auditctl as unprivileged user */
    if (!success) {
	rc = setresuid_test();
	if (rc < 0)
	    goto exit;
	context->experror = -EINTR;
    }

    rc = context_setidentifiers(context);
    if (rc < 0)
	goto exit;

    context_setbegin(context);
    pid = fork();
    if (pid < 0) {
	rc = -1;
	fprintf(stderr, "Error: fork(): %s\n", strerror(errno));
	goto exit;
    }
    if (pid == 0) {
	execve("/sbin/auditctl", argv, NULL);
	/* not reached due to EINTR */
    } else {
	if (waitpid(pid, &status, 0) < 0) {
	    rc = -1;
	    fprintf(stderr, "Error: waitpid(): %s\n", strerror(errno));
	    goto exit;
	}
    }
    context_setend(context);
    
    exit = WEXITSTATUS(status);
    context->pid = pid;
    context->success = (exit == 0);
    context->error = context->u.syscall.exit = exit;

exit:
    if (!success)
	if (setresuid(0, 0, 0) < 0) /* clean up from failure case */
	    fprintf(stderr, "Warning: can't switch back to root\n");
    return rc;
}
