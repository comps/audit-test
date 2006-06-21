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
 *  test_msgget.c
 *
 *  PURPOSE:
 *  Verify audit of attempts to create/get message queue identifiers.
 *
 *  SYSCALLS:
 *  msgget()
 *
 *  TESTCASE: successful
 *  Get identifier for an existing message queue.
 *
 *  TESTCASE: unsuccessful
 *  Attempt to create a new message queue with the key of an existing
 *  message queue (specifying IPC_EXCL).
 */

#include "includes.h"
#include "syscalls.h"
#include <sys/ipc.h>
#if defined(__powerpc64__)
#include <asm-ppc64/ipc.h>
#elif !defined(__ia64__)
#include <asm-generic/ipc.h>
#endif
#include <sys/msg.h>

int test_msgget(struct audit_data *context, int variation, int success)
{
    int rc = 0;
    int msgflag = S_IRWXU|IPC_CREAT;
    int qid;
    int exit;

    errno = 0;
    rc = qid = msgget(TEST_IPC_KEY, msgflag);
    if (rc < 0) {
        fprintf(stderr, "Error: can't create message queue: %s\n",
                strerror(errno));
        goto exit;
    }
    fprintf(stderr, "Message queue key: %d id: %d\n", TEST_IPC_KEY, qid);

    if (!success) {
        msgflag |= IPC_EXCL;
        context_setexperror(context, EEXIST);
    }

    rc = context_setidentifiers(context);
    if (rc < 0)
        goto exit_queue;
#if defined (__i386)
    context_setarg(context, 0, MSGGET);
#endif

    context_setbegin(context);
#if defined (__x86_64) || defined (__ia64)
    fprintf(stderr, "Attempting %s(%x, %x)\n", 
            context->u.syscall.sysname, TEST_IPC_KEY, msgflag);
#else
    fprintf(stderr, "Attempting %s(%x, %x, %x)\n", 
            context->u.syscall.sysname, MSGGET, TEST_IPC_KEY, msgflag);
#endif
    errno = 0;
    exit = msgget(TEST_IPC_KEY, msgflag);
    context_setend(context);
    context_setresult(context, exit, errno);

    fprintf(stderr, "Message queue id: %d\n", exit);

exit_queue:
    errno = 0;
    if (msgctl(qid, IPC_RMID, 0) < 0)
        fprintf(stderr, "Error: removing message queue: %s\n", strerror(errno));

exit:
    return rc;
}
