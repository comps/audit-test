/*  (c) Copyright Hewlett-Packard Development Company, L.P., 2007
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of version 2 the GNU General Public License as
 *  published by the Free Software Foundation.
 *  
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *  
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "includes.h"
#include <sys/msg.h>
#include <sys/sem.h>
#include <sys/shm.h>

/* from <linux/ipc.h> which has conflicting definition of ipc_perm */
/* NOTE that these defines are (at this time) used only internally
 * for one specific switch{} in do_ipc.c */
#define SEMOP            1
#define SEMGET           2
#define SEMCTL           3
#define SEMTIMEDOP       4
#define MSGSND          11
#define MSGRCV          12
#define MSGGET          13
#define MSGCTL          14
#define SHMAT           21
#define SHMDT           22
#define SHMGET          23
#define SHMCTL          24

int check_ipc_usage(char *call, int nargs)
{
    if (!strcmp(call, "msgctl") || !strcmp(call, "msgget") ||
	!strcmp(call, "semctl") || !strcmp(call, "semget") ||
	!strcmp(call, "shmctl") | !strcmp(call, "shmget")) {
	if (nargs != 3) {
	    fprintf(stderr, "%s <ipc_key> <flag>\n", call);
	    return 1;
	}
    } else if (!strcmp(call, "msgrcv")) {
	if (nargs != 3) {
	    fprintf(stderr, "%s <qid> <msg_type>\n", call);
	    return 1;
	}
    } else if (!strcmp(call, "msgsnd")) {
	if (nargs != 4) {
	    fprintf(stderr, "%s <qid> <msg_type> <msg_string>\n", call);
	    return 1;
	}
    } else if (!strcmp(call, "semop") || !strcmp(call, "semtimedop")) {
	if (nargs != 3) {
	    fprintf(stderr, "%s <semid> <flag>\n", call);
	    return 1;
	}
    } else if (!strcmp(call, "shmat")) {
	if (nargs != 3) {
	    fprintf(stderr, "%s <shmid> <flag>\n", call);
	    return 1;
	}
    } else {
	fprintf(stderr, "%s is not a supported ipc operation\n", call);
	return 1;
    }

    return 0;
}

int translate_ipc_flags(char *string, int *flags)
{
    if (!strcmp(string, "create"))
	/* use IPC_EXCL on create to catch cleanup problems */
	*flags |= IPC_CREAT|IPC_EXCL|S_IRUSR|S_IWUSR;
    else if (!strncmp(string, "create:", strlen("create:"))) {
        /* create with custom mode */
        *flags |= IPC_CREAT|IPC_EXCL;
        *flags &= ~((int)0x01ff);  /* reset 9 lsbits */
        *flags |= strtol(string+strlen("create:"), NULL, 8);
    }
    else if (!strcmp(string, "read"))
	*flags |= S_IRUSR;
    else if (!strcmp(string, "write"))
	*flags |= S_IWUSR;
    else if (!strcmp(string, "rdwr"))
	*flags |= S_IRUSR|S_IWUSR;
    else if (!strcmp(string, "remove"))
	*flags |= IPC_RMID;
    else if (!strcmp(string, "set"))
	*flags |= IPC_SET;
    else
        *flags |= atoi(string);

    return 0;
}

int translate_sem_flags(char *string, int *flags)
{
    if (!strcmp(string, "read"))
	*flags = 0;
    else if (!strcmp(string, "write"))
	*flags = 1;
    else {
	fprintf(stderr, "sem flag must be one of <read|write>\n");
	return 1;
    }

    return 0;
}

int translate_shm_flags(char *string, int *flags)
{
    if (!strcmp(string, "read"))
	*flags = SHM_RDONLY;
    else if (!strcmp(string, "write"))
	*flags = 0;
    else {
	fprintf(stderr, "shm flag must be one of <read|write>\n");
	return 1;
    }

    return 0;
}

int translate_ipc_op(char *string, int *op)
{
    if (!strcmp(string, "msgctl"))
	*op = MSGCTL;
    else if (!strcmp(string, "msgget"))
	*op = MSGGET;
    else if (!strcmp(string, "msgrcv"))
	*op = MSGRCV;
    else if (!strcmp(string, "msgsnd"))
	*op = MSGSND;
    else if (!strcmp(string, "semctl"))
	*op = SEMCTL;
    else if (!strcmp(string, "semget"))
	*op = SEMGET;
    else if (!strcmp(string, "semop"))
	*op = SEMOP;
    else if (!strcmp(string, "semtimedop"))
	*op = SEMTIMEDOP;
    else if (!strcmp(string, "shmctl"))
	*op = SHMCTL;
    else if (!strcmp(string, "shmget"))
	*op = SHMGET;
    else if (!strcmp(string, "shmat"))
	*op = SHMAT;
    else {
	fprintf(stderr, "%s is not a supported ipc operation\n", string);
	return 1;
    }

    return 0;
}

int do_msgctl(int msqid, int cmd)
{
    struct msqid_ds buf;
    int ret = -1;

    switch (cmd) {
    case IPC_RMID:
	ret = msgctl(msqid, cmd, NULL);
	break;
    case IPC_SET:
	memset(&buf, 0, sizeof(buf));
	buf.msg_perm.uid = 0; /* use root's uid */
	ret = msgctl(msqid, cmd, &buf);
	break;
    case IPC_STAT:
	ret = msgctl(msqid, cmd, &buf);
	break;
    }
    return ret;
}

int do_msgget(key_t key, int flags)
{
    return msgget(key, flags);
}

int do_msgrcv(int msqid, long msgtype)
{
#define MAX_MSG 256
    struct msgbuf *buf;
    int buflen, exitval;

    buflen = sizeof(struct msgbuf) + MAX_MSG;
    buf = (struct msgbuf *)malloc(buflen);
    if (!buf) {
	perror("malloc");
	return -1;
    }

    exitval = msgrcv(msqid, buf, buflen, msgtype, IPC_NOWAIT);
    free(buf);

    return exitval;
}

int do_msgsnd(int msqid, long msgtype, char *msg)
{
    struct msgbuf *buf;
    int buflen, exitval;

    buflen = sizeof(struct msgbuf) + strlen(msg) + 1;
    buf = (struct msgbuf *)malloc(buflen);
    if (!buf) {
	perror("malloc");
	return -1;
    }
    buf->mtype = msgtype;
    strcpy(buf->mtext, msg);

    exitval = msgsnd(msqid, buf, buflen, IPC_NOWAIT);
    free(buf);

    return exitval;
}

int do_semctl(int semid, int cmd)
{
   union semun
   {
     int val;
     struct semid_ds *buf;
     unsigned short int *array;
     struct seminfo *__buf;
   };

    union semun sebuf;
    struct semid_ds tmpbuf;
    int ret = -1;

    switch (cmd) {
    case IPC_RMID:
	ret = semctl(semid, 1, cmd, NULL);
	break;
    case IPC_SET:
	memset(&sebuf, 0, sizeof(sebuf));
        sebuf.buf = &tmpbuf;
	((struct semid_ds *)sebuf.buf)->sem_perm.uid = 0; /* use root's uid */
	ret = semctl(semid, 1, cmd, sebuf);
	break;
    case IPC_STAT:
	ret = semctl(semid, 1, cmd, &sebuf);
	break;
    }
    return ret;
}

int do_semget(key_t key, int flags)
{
    /* number of semaphores = 1 */
    return semget(key, 1, flags);
}

int do_semop(int semid, int op)
{
    struct sembuf sops;

    sops.sem_num = 0;
    sops.sem_op = op;
    sops.sem_flg = SEM_UNDO;

    /* number of semaphores = 1 */
    return semop(semid, &sops, 1);
}

int do_semtimedop(int semid, int op)
{
    struct sembuf sops;
    struct timespec timeout = { 1, 0 };

    sops.sem_num = 0;
    sops.sem_op = op;
    sops.sem_flg = SEM_UNDO;

    /* number of semaphores = 1 */
    return semtimedop(semid, &sops, 1, &timeout);
}

int do_shmctl(int shmid, int cmd)
{
    struct shmid_ds buf;
    int ret = -1;

    switch (cmd) {
    case IPC_RMID:
	ret = shmctl(shmid, cmd, NULL);
	break;
    case IPC_SET:
	memset(&buf, 0, sizeof(buf));
	buf.shm_perm.uid = 0; /* use root's uid */
	ret = shmctl(shmid, cmd, &buf);
	break;
    case IPC_STAT:
	ret = shmctl(shmid, cmd, &buf);
	break;
    }
    return ret;
}

int do_shmget(key_t key, int flags)
{
    return shmget(key, getpagesize(), flags);
}

long do_shmat(int shmid, int op)
{
    void *shmadd;
    int saved_errno;

    shmadd = shmat(shmid, NULL, op);
    saved_errno = errno;

    if (((unsigned long)shmadd != (unsigned long)-1) &&
	(shmdt(shmadd) < 0))
	fprintf(stderr, "Warning: can't detach from shared memory! %s\n",
		strerror(errno));

    errno = saved_errno;
    return (long)shmadd;
}
