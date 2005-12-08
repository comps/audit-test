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
 *  test_msgctl.c
 *
 *  PURPOSE:
 *  Verify audit of attempts perform message control operations.
 *
 *  SYSCALLS:
 *  msgctl()
 *
 *  TESTCASE: remove successful
 *  Remove a message queue.
 *
 *  TESTCASE: remove unsuccessful
 *  Attempt to remove a message queue with insufficient access
 *  permissions.
 *
 *  TESTCASE: setperms successful
 *  Change message queue permissions.
 *
 *  TESTCASE: setperms unsuccessful
 *  Attempt to change message queue permissions, while having
 *  insufficient access permissions.
 */

#include "includes.h"
#include "syscalls.h"
#include <sys/ipc.h>
#if defined(__powerpc64__)
#include <asm-ppc64/ipc.h>
#elif !defined(__ia64__)
#include <asm/ipc.h>
#endif
#include <sys/msg.h>

static int test_msgctl_setperms(struct audit_data *context, int success)
{
    int rc = 0;
    int qid;
    struct msqid_ds buf;
    int exit;

    errno = 0;
    rc = qid = msgget(IPC_PRIVATE, S_IRWXU|IPC_CREAT);
    if (rc < 0) {
	fprintf(stderr, "Error: can't create message queue: %s\n",
		strerror(errno));
        goto exit;
    }
    fprintf(stderr, "Message queue key: %d id: %d\n", IPC_PRIVATE, qid);

    if (!success) {
	rc = seteuid_test();
	if (rc < 0)
	    goto exit_queue;
        context_setexperror(context, EPERM);
    }

    rc = context_setidentifiers(context);
    if (rc < 0)
        goto exit_root;

    memset(&buf, 0, sizeof(buf));
    buf.msg_perm.uid = gettestuid();
    context_setipc(context, buf.msg_qbytes, buf.msg_perm.uid,
		   buf.msg_perm.gid, buf.msg_perm.mode);

    errno = 0;
    context_setbegin(context);
    fprintf(stderr, "Attempting msgctl(%d, IPC_SET)\n", qid);
    exit = msgctl(qid, IPC_SET, &buf);
    context_setend(context);

    if (exit < 0) {
        context->success = 0;
        context->error = context->u.syscall.exit = -errno;
    } else {
        context->success = 1;
        context->u.syscall.exit = exit;
    }

exit_root:
    if (!success && seteuid(0) < 0)
	fprintf(stderr, "Error: seteuid(0): %s\n", strerror(errno));

exit_queue:
    if (msgctl(qid, IPC_RMID, NULL) < 0)
	fprintf(stderr, "Error: removing message queue: %s\n", strerror(errno));

exit:
    return rc;
}

static int test_msgctl_remove(struct audit_data *context, int success)
{
    int rc = 0;
    int qid;
    int exit = -1; /* pre-set for proper cleanup */

    errno = 0;
    rc = qid = msgget(IPC_PRIVATE, S_IRWXU|IPC_CREAT);
    if (rc < 0) {
	fprintf(stderr, "Error: can't create message queue: %s\n",
		strerror(errno));
        goto exit;
    }
    fprintf(stderr, "Message queue key: %d id: %d\n", IPC_PRIVATE, qid);

    if (!success) {
	rc = seteuid_test();
	if (rc < 0)
	    goto exit_queue;
        context_setexperror(context, EPERM);
    }

    rc = context_setidentifiers(context);
    if (rc < 0)
        goto exit_root;

    errno = 0;
    context_setbegin(context);
    fprintf(stderr, "Attempting msgctl(%d, IPC_RMID)\n", qid);
    exit = msgctl(qid, IPC_RMID, NULL);
    context_setend(context);

    if (exit < 0) {
        context->success = 0;
        context->error = context->u.syscall.exit = -errno;
    } else {
        context->success = 1;
        context->u.syscall.exit = exit;
    }

exit_root:
    if (!success && seteuid(0) < 0)
	fprintf(stderr, "Error: seteuid(0): %s\n", strerror(errno));

exit_queue:
    if (exit < 0 && msgctl(qid, IPC_RMID, NULL) < 0)
	fprintf(stderr, "Error: removing message queue: %s\n", strerror(errno));

exit:
    return rc;
}

int test_msgctl(struct audit_data *context, int variation, int success)
{
    switch(variation) {
    case SYSCALL_REMOVE:
	return test_msgctl_remove(context, success);
    case SYSCALL_SETPERMS:
	return test_msgctl_setperms(context, success);
    default:
	fprintf(stderr, "Test variation [%i] unsupported for %s()\n", 
		variation, context->u.syscall.sysname);
	return -1;
    }
}
