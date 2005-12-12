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
		context->error, strerror(-context->error));
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
		context->experror, context->error, strerror(-context->error));
    }

exit:
    return rc;
}

/* Verify presence or absence of log record */
ts_exit verify_logresult(struct audit_data *context)
{
    ts_exit rc;
    char cmd[512] = {0};
    int count = 0;

    if (context->type & AUDIT_MSG_SYSCALL) {
	count = snprintf(cmd, sizeof(cmd), "augrok -m1 'type=~SYSCALL' "
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
	if (count < sizeof(cmd) && context->type & AUDIT_MSG_SOCKADDR) {
	    count += snprintf(&cmd[count], sizeof(cmd)-count, 
			      " saddr=%s", 
			      context->u.syscall.sockaddr); 
	}
	if (count < sizeof(cmd) && context->type & AUDIT_MSG_CWD) {
	    count += snprintf(&cmd[count], sizeof(cmd)-count, 
			      " cwd==%s", 
			      context->u.syscall.fs_cwd);
	}
	if (count < sizeof(cmd) && context->type & AUDIT_MSG_WATCH) {
	    count += snprintf(&cmd[count], sizeof(cmd)-count, 
			      " filterkey==%s", 
			      context->u.syscall.fs_watch);
	}
	if (count < sizeof(cmd) && context->type & AUDIT_MSG_PATH) {
	    if (strcmp(context->u.syscall.fs_sobj, ""))
		count += snprintf(&cmd[count], sizeof(cmd)-count, 
				  " name==%s name_1==%s", 
				  context->u.syscall.fs_sobj,
				  context->u.syscall.fs_tobj);
	    else
		count += snprintf(&cmd[count], sizeof(cmd)-count, 
				  " name==%s", 
				  context->u.syscall.fs_tobj);
	}

	if (count < sizeof(cmd) && context->type & AUDIT_MSG_ARG0) {
	    count += snprintf(&cmd[count], sizeof(cmd)-count, 
			      " a0==%x", 
			      context->u.syscall.arg[0]);
	}
	if (count < sizeof(cmd) && context->type & AUDIT_MSG_ARG1) {
	    count += snprintf(&cmd[count], sizeof(cmd)-count, 
			      " a1==%x", 
			      context->u.syscall.arg[1]);
	}
	if (count < sizeof(cmd) && context->type & AUDIT_MSG_ARG2) {
	    count += snprintf(&cmd[count], sizeof(cmd)-count, 
			      " a2==%x", 
			      context->u.syscall.arg[2]);
	}
	if (count < sizeof(cmd) && context->type & AUDIT_MSG_ARG3) {
	    count += snprintf(&cmd[count], sizeof(cmd)-count, 
			      " a3==%x", 
			      context->u.syscall.arg[3]);
	}
    } else if (context->type & AUDIT_MSG_USER) {
	count = snprintf(cmd, sizeof(cmd), "augrok -m1 'type=~USER' "
			 "msg_1=~\"%s\"",
			 context->comm);
    } else if (context->type & AUDIT_MSG_DAEMON) {
	count = snprintf(cmd, sizeof(cmd), "augrok -m1 msg_1=~\"%s\"",
			 context->comm);
    }

    if (count >= sizeof(cmd)) {
	fprintf(stderr, "ERROR: verify_logresult: cmd too long\n");
	rc = TEST_ERROR;
	goto exit;
    }

    rc = (system(cmd) == 0) ? TEST_EXPECTED : TEST_UNEXPECTED;
    if (rc == TEST_UNEXPECTED)
	fprintf(stderr, "Expected record not found in log:\n%s\n", cmd);

exit:
    return rc;
}
