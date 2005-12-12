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

int test_msgctl_set(struct audit_data *context, int variation, int success)
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
#if defined (__x86_64) || defined (__ia64)
    context_setarg(context, 1, IPC_SET);
#else
    context_setarg(context, 0, MSGCTL);
    context_setarg(context, 2, IPC_SET | 0x100); /* unknown flag set in call */
#endif

    memset(&buf, 0, sizeof(buf));
    buf.msg_perm.uid = gettestuid();
    context_setipc(context, buf.msg_qbytes, buf.msg_perm.uid,
		   buf.msg_perm.gid, buf.msg_perm.mode);

    context_setbegin(context);
#if defined (__x86_64) || defined (__ia64)
    fprintf(stderr, "Attempting %s(%x, %x, %p)\n", 
	    context->u.syscall.sysname, qid, IPC_SET, &buf);
#else
    fprintf(stderr, "Attempting %s(%x, %x, %x, %p)\n", 
	    context->u.syscall.sysname, MSGCTL, qid, IPC_SET, &buf);
#endif
    errno = 0;
    exit = msgctl(qid, IPC_SET, &buf);
    context_setend(context);
    context_setresult(context, exit, errno);

exit_root:
    errno = 0;
    if (!success && (seteuid(0) < 0))
	fprintf(stderr, "Error: seteuid(): %s\n", strerror(errno));

exit_queue:
    errno = 0;
    if (msgctl(qid, IPC_RMID, NULL) < 0)
	fprintf(stderr, "Error: removing message queue: %s\n", strerror(errno));

exit:
    return rc;
}

int test_msgctl_rmid(struct audit_data *context, int variation, int success)
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
#if defined (__x86_64) || defined (__ia64)
    context_setarg(context, 1, IPC_RMID);
#else
    context_setarg(context, 0, MSGCTL);
    context_setarg(context, 2, IPC_RMID | 0x100); /* unknown flag set in call */
#endif

    context_setbegin(context);
#if defined (__x86_64) || defined (__ia64)
    fprintf(stderr, "Attempting %s(%x, %x, %p)\n", 
	    context->u.syscall.sysname, qid, IPC_RMID, NULL);
#else
    fprintf(stderr, "Attempting %s(%x, %x, %x, %p)\n", 
	    context->u.syscall.sysname, MSGCTL, qid, IPC_RMID, NULL);
#endif
    errno = 0;
    exit = msgctl(qid, IPC_RMID, NULL);
    context_setend(context);
    context_setresult(context, exit, errno);

exit_root:
    errno = 0;
    if (!success && (seteuid(0) < 0))
	fprintf(stderr, "Error: seteuid(): %s\n", strerror(errno));

exit_queue:
    errno = 0;
    if (exit < 0 && msgctl(qid, IPC_RMID, NULL) < 0)
	fprintf(stderr, "Error: removing message queue: %s\n", strerror(errno));

exit:
    return rc;
}
