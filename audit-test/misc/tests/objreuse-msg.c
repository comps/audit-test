/* objreuse-msg.c - message queue allocated w/ shmget must be empty.
 *
 * $Id: objreuse-msg.c,v 1.1 2004/06/23 22:48:21 danjones Exp $
 *
 * Purpose: verify that the object reuse mechanism works as documented
 * 	in the case of message queues returned by the kernel. All message
 *	queues must be empty.
 *
 * Method: create a message queue via the msgget() system call and verify
 *      the number of messages (msg_qnum) and number of bytes (msq_bytes)
 *      is 0 in the message queue structure.
 *
 * Expected result: exit code 0 indidates that all tests worked as
 *	expected.
 *
 */

#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>
#include <stdarg.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/stat.h>
#include <sys/errno.h>

#include "testmacros.h"

#define MSGMNB 16384

int main(int argc, char *argv[])
{

  int rc;
  int msgid;
  struct msqid_ds msqstat;
  struct msgbuf
  {
    long mtype;
    char mtext[MSGMNB];
  } r_message;

  SYSCALL( msgid = msgget(IPC_PRIVATE, 0) );
  SYSCALL( msgctl(msgid, IPC_STAT, &msqstat) );
  DIE_IF( msqstat.msg_qnum != 0 );
  DIE_IF( msqstat.msg_cbytes != 0 );
  rc = msgrcv(msgid, &r_message, MSGMNB, 0, IPC_NOWAIT);
  DIE_IF( (rc != -1) || (errno != ENOMSG) );
  SYSCALL( msgctl(msgid, IPC_RMID, NULL) );

  printf("%s: PASS\n", __FILE__);
  return 0;
}
