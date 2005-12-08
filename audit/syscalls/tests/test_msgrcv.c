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
 *  test_msgrcv.c
 *
 *  PURPOSE:
 *  Verify audit of attempts to read messages from a message queue.
 *
 *  SYSCALLS:
 *  msgrcv()
 *
 *  TESTCASE: successful
 *  Read a message from a message queue.
 *
 *  TESTCASE: unsuccessful
 *  Attempt to read a message from a message queue with
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

int test_msgrcv(struct audit_data *context, int variation, int success)
{
    int rc = 0;
    int qid;
    char *msg = "test message";
    struct msgbuf *buf;	/* used both to send & recv message */
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

    buflen = sizeof(struct msgbuf) + strlen(msg) + 1;
    buf = (struct msgbuf *)malloc(buflen);
    if (!buf) {
	fprintf(stderr, "Error: malloc(): %s\n", strerror(errno));
	rc = -1;
	goto exit_queue;
    }
    buf->mtype = TEST_MSG_TYPE;
    strcpy(buf->mtext, msg);
    rc = msgsnd(qid, buf, buflen, IPC_NOWAIT);
    if (rc < 0) {
	fprintf(stderr, "Error: can't add message to queue: %s\n",
		strerror(errno));
        goto exit_free;
    }

    if (!success) {
	rc = seteuid_test();
	if (rc < 0)
	    goto exit_free;
	context_setexperror(context, EACCES);
    }

    rc = context_setidentifiers(context);
    if (rc < 0)
        goto exit_root;

    context_setbegin(context);
    fprintf(stderr, "Attempting %s(%x, %p, %x, %x, IPC_NOWAIT)\n", 
	    context->u.syscall.sysname, qid, buf, buflen, TEST_MSG_TYPE);
    errno = 0;
    exit = msgrcv(qid, buf, buflen, TEST_MSG_TYPE, IPC_NOWAIT);
    context_setend(context);
    context_setresult(context, exit, errno);

exit_root:
    errno = 0;
    if (!success && (seteuid(0) < 0))
	fprintf(stderr, "Error: seteuid(): %s\n", strerror(errno));

exit_free:
    free(buf);

exit_queue:
    errno = 0;
    if (msgctl(qid, IPC_RMID, 0) < 0)
	fprintf(stderr, "Error: removing message queue: %s\n", strerror(errno));

exit:
    return rc;
}
