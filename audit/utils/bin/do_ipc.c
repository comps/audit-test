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
#include <sys/ipc.h>
#include <asm-generic/ipc.h>

#include "ipc_common.c"

int main(int argc, char **argv)
{
    int exitval, result;
    int op, flags = 0;

    if (argc < 1) {
	fprintf(stderr, "%s: you must specify an ipc operation\n", argv[0]);
	return 1;
    }

    if (check_ipc_usage(argv[1], argc - 1))
	return 1;

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

    printf("%d %d %d\n", result, result ? errno : exitval, getpid());
    return result;
}
