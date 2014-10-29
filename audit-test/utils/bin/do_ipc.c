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

int main(int argc, char **argv)
{
    int exitval, result;
    int op, flags = 0;

    if (argc < 2) {
	fprintf(stderr, "%s: you must specify an ipc operation\n", argv[0]);
	return 1;
    }

    if (translate_ipc_op(argv[1], &op))
	return 1;

    switch (op) {
    case MSGCTL:
    case MSGGET:
    case SEMCTL:
    case SEMGET:
    case SHMCTL:
    case SHMGET:
	if (translate_ipc_flags(argv[3], &flags))
	    return 1;
	break;
    case SEMOP:
    case SEMTIMEDOP:
	if (translate_sem_flags(argv[3], &flags))
	    return 1;
	break;
    case SHMAT:
	if (translate_shm_flags(argv[3], &flags))
	    return 1;
	break;
    }

    errno = 0;
    switch (op) {
    case MSGCTL:
	exitval = do_msgctl(atoi(argv[2]), flags);
	break;
    case MSGGET:
	exitval = do_msgget(atoi(argv[2]), flags);
	break;
    case MSGRCV:
	exitval = do_msgrcv(atoi(argv[2]), atoi(argv[3]));
	break;
    case MSGSND:
	exitval = do_msgsnd(atoi(argv[2]), atoi(argv[3]), argv[4]);
	break;
    case SEMCTL:
	exitval = do_semctl(atoi(argv[2]), flags);
	break;
    case SEMGET:
	exitval = do_semget(atoi(argv[2]), flags);
	break;
    case SEMOP:
	exitval = do_semop(atoi(argv[2]), flags);
	break;
    case SEMTIMEDOP:
	exitval = do_semtimedop(atoi(argv[2]), flags);
	break;
    case SHMCTL:
	exitval = do_shmctl(atoi(argv[2]), flags);
	break;
    case SHMGET:
	exitval = do_shmget(atoi(argv[2]), flags);
	break;
    case SHMAT:
	exitval = do_shmat(atoi(argv[2]), flags);
	break;
    default:
	fprintf(stderr, "%s: %d is not a supported ipc operation\n", argv[0], op);
	return 1;
    }

    switch(op) {
    case SHMAT:
	result = exitval == -1;
	break;
    default:
	result = exitval < 0;
    }

    switch(op) {
    case MSGGET:
    case SEMGET:
    case SHMGET:
        if (result == 0)
            printf("%d\n", exitval);
        break;
    default:
        break;
    }

    fprintf(stderr, "%d %d %d\n", result, result ? errno : exitval, getpid());
    return result;
}
