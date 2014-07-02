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
#include <sys/sem.h>

union semun
{
    int val;
    struct semid_ds *buf;
    unsigned short int *array;
    struct seminfo *__buf;
};

int main(int argc, char **argv)
{
    int exitval, result;
    int semid, cmd = 0;
    union semun sebuf;
    struct semid_ds tmpbuf;

    if (argc != 3) {
        fprintf(stderr, "Usage:\n%s <semid> <cmd>\n", argv[0]);
        return 1;
    }

    semid = atoi(argv[1]);
    if (!strcmp(argv[2], "remove"))
        cmd |= IPC_RMID;
    else if (!strcmp(argv[2], "set"))
        cmd |= IPC_SET;
    else if (!strcmp(argv[2], "stat"))
        cmd |= IPC_STAT;
    else
        cmd |= atoi(argv[2]);

    errno = 0;

    switch (cmd) {
    case IPC_RMID:
        exitval = semctl(semid, 1, cmd, NULL);
        break;
    case IPC_SET:
        memset(&sebuf, 0, sizeof(sebuf));
        sebuf.buf = &tmpbuf;
        ((struct semid_ds *)sebuf.buf)->sem_perm.uid = 0; /* use root's uid */
        exitval = semctl(semid, 1, cmd, sebuf);
        break;
    case IPC_STAT:
        exitval = semctl(semid, 1, cmd, &sebuf);
        break;
    default:
        exitval = -1;
        break;
    }

    result = exitval < 0;

    fprintf(stderr, "%d %d %d\n", result, result ? errno : exitval, getpid());
    return result;
}
