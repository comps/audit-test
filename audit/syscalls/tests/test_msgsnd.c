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
 *  test_msgsnd.c
 *
 *  PURPOSE:
 *  Verify audit of attempts to append messages to a message queue.
 *
 *  SYSCALLS:
 *  msgsnd()
 *
 *  TESTCASE: successful
 *  Append a message to a message queue.
 *
 *  TESTCASE: unsuccessful
 *  Attempt to append a message to a message queue with insufficient
 *  access permissions.
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

int test_msgsnd(struct audit_data *context, int variation, int success)
{
    int rc = 0;
    int qid;
    char *msg = "test message";
    struct msgbuf *buf;
    int buflen;
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
	context->experror = -EACCES;
    }

    rc = context_setidentifiers(context);
    if (rc < 0)
        goto exit_root;

    buflen = sizeof(struct msgbuf) + strlen(msg) + 1;
    buf = (struct msgbuf *)malloc(buflen);
    if (!buf) {
	fprintf(stderr, "Error: malloc(): %s\n", strerror(errno));
	rc = -1;
	goto exit_root;
    }
    buf->mtype = TEST_MSG_TYPE;
    strcpy(buf->mtext, msg);

    errno = 0;
    context_setbegin(context);
    fprintf(stderr, "Attempting msgsnd(%i, {%li, \"%s\"}, %i, IPC_NOWAIT)\n", 
	    qid, buf->mtype, buf->mtext, buflen);
    exit = msgsnd(qid, buf, buflen, IPC_NOWAIT);
    context_setend(context);

    if (exit < 0) {
        context->success = 0;
        context->error = context->u.syscall.exit = -errno;
    } else {
        context->success = 1;
        context->u.syscall.exit = exit;
    }

    free(buf);
exit_root:
    if (!success && seteuid(0) < 0)
	fprintf(stderr, "Error: seteuid(0): %s\n", strerror(errno));

exit_queue:
    if (msgctl(qid, IPC_RMID, 0) < 0)
	fprintf(stderr, "Error: removing message queue: %s\n", strerror(errno));

exit:
    return rc;
}
