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
    else if (!strcmp(string, "stat"))
        *flags |= IPC_STAT;
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
