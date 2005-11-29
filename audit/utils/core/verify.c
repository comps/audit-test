/* =======================================================================
 * Copyright (C) 2005 Hewlett-Packard Company
 * 
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License.
 * 
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 * 
 *   You should have received a copy of the GNU General Public License
 *   with this package; if not, write to the Free Software Foundation,
 *   Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 * 
 * ======================================================================= 
 */

#include "includes.h"

/* Verify operation result */
ts_exit verify_opresult(struct audit_data *context, int success)
{
    ts_exit rc = TEST_EXPECTED;

    if (success && !context->success) {
	rc = TEST_UNEXPECTED;
	fprintf(stderr, 
		"Expected operation success, got operation failure: [%d] %s\n",
		context->error, strerror(context->error));
	goto exit;
    }

    if (!success && context->success) {
	rc = TEST_UNEXPECTED;
	fprintf(stderr, "Expected operation failure, got operation success.\n");
	goto exit;
    }

    if (context->experror && context->experror != context->error) {
	rc = TEST_UNEXPECTED;
	fprintf(stderr,
		"Expected operation error [%d], got operation error [%d] %s\n",
		context->experror, context->error, strerror(context->error));
    }

exit:
    return rc;
}

/* Verify presence or absence of log record */
ts_exit verify_logresult(struct audit_data *context)
{
    ts_exit rc;
    char cmd[512] = {0};
    int count;

    if (context->type & AUDIT_MSG_SYSCALL) {
	count = snprintf(cmd, sizeof(cmd), "augrep -m1 'type=~SYSCALL' "
			 "syscall==%d 'success==%s' " 
			 "exit==%d pid==%d auid==%u uid==%u gid==%u " 
			 "euid==%u suid==%u fsuid==%u " 
			 "egid==%u sgid==%u fsgid==%u", 
			 context->u.syscall.sysnum, 
			 context->success ? "yes" : "no", 
			 context->u.syscall.exit, 
			 context->pid, 
			 context->loginuid, 
			 context->uid, 
			 context->gid, 
			 context->euid, 
			 context->suid, 
			 context->fsuid, 
			 context->egid, 
			 context->sgid, 
			 context->fsgid);
	if (count < sizeof(cmd) && context->type & AUDIT_MSG_IPC) {
	    count += snprintf(&cmd[count], sizeof(cmd)-count, 
			      " iuid==%u mode==%x qbytes==%x igid==%u", 
			      context->u.syscall.ipc_uid, 
			      context->u.syscall.ipc_mode, 
			      context->u.syscall.ipc_qbytes, 
			      context->u.syscall.ipc_gid);
	}
	if (count < sizeof(cmd) && context->type & AUDIT_MSG_CWD) {
	    count += snprintf(&cmd[count], sizeof(cmd)-count, 
			      " cwd==%s filterkey==%s", 
			      context->u.syscall.fs_cwd,
			      context->u.syscall.fs_tobj);
	}
	if (count >= sizeof(cmd)) {
	    fprintf(stderr, "ERROR: verify_logresult: cmd too long\n");
	    rc = TEST_ERROR;
	    goto exit;
	}
    }

    rc = (system(cmd) == 0) ? TEST_EXPECTED : TEST_UNEXPECTED;
    if (rc == TEST_UNEXPECTED)
	fprintf(stderr, "Expected record not found in log:\n%s\n", cmd);

exit:
    return rc;
}
