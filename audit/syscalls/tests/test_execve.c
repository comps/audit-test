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
 *  Execute /bin/true.
 *
 *  TESTCASE: unsuccessful
 *  Attempt to execute a file which does have execute access
 *  permission.
 */

#include "includes.h"
#include "syscalls.h"
#include <sys/wait.h>

int test_execve(struct audit_data *context, int variation, int success)
{
    int rc = 0;
    char *cmd = NULL;
    pid_t pid;
    int status, exit;

    rc = context_setidentifiers(context);
    if (rc < 0)
	goto exit;

    if (success)
	cmd = "/bin/true";
    else {
	rc = createTempFile(&cmd, S_IRUSR|S_IRGRP|S_IROTH,
			    context->euid, context->egid);
	if (rc < 0) {
	    fprintf(stderr, "Error: cannot create tmp file: %s\n", strerror(errno));
	    goto exit;
	}
	context->experror = -EACCES;
    }

    context_setbegin(context);
    pid = fork();
    if (pid < 0) {
	rc = -1;
	fprintf(stderr, "Error: fork(): %s\n", strerror(errno));
	goto exit_free;
    }
    if (pid == 0) {
	execve(cmd, NULL, NULL);
	_exit(errno);
    } else {
	if (waitpid(pid, &status, 0) < 0) {
	    rc = -1;
	    fprintf(stderr, "Error: waitpid(): %s\n", strerror(errno));
	    goto exit_free;
	}
    }
    context_setend(context);
    
    exit = WEXITSTATUS(status);
    context->pid = pid;
    context->success = (exit == 0);
    context->error = context->u.syscall.exit = -exit;

exit_free:
    if (!success)
	free(cmd);

exit:
    return rc;
}
