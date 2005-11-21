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
    ts_exit rc = TEST_EXPECTED;

    if (context->type & AUDIT_MSG_SYSCALL) {
	/* log fields to check:
	 *
	 * type =~ SYSCALL
	 * syscall == context->u.syscall.sysnum
	 * success == context->success ? "yes" : "no"
	 * exit == context->u.syscall.exit
	 * pid == context->pid
	 * auid == context->loginuid
	 * uid == context->uid
	 * gid == context->gid
	 * euid == context->euid
	 * suid == context->suid
	 * fsuid == context->fsuid
	 * egid == context->egid
	 * sgid == context->sgid
	 * fsgid == context->fsgid
	 */

	/* if record found goto exit */
    }

    rc = TEST_UNEXPECTED;
    fprintf(stderr, "Expected record not found in log\n");

//exit:
    return rc;
}
