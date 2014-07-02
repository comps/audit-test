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

int main(int argc, char **argv)
{
    int exitval, result;
    int flags = 0;
    struct sembuf sops;

    if (argc != 3) {
        fprintf(stderr, "Usage:\n%s <semid> <op>\n", argv[0]);
        return 1;
    }

    if (!strcmp(argv[2], "read"))
        flags = 0;
    else if (!strcmp(argv[2], "write"))
        flags = 1;
    else {
        fprintf(stderr, "op must be one of <read|write>\n");
        return 1;
    }

    sops.sem_num = 0;
    sops.sem_op = flags;
    sops.sem_flg = SEM_UNDO;

    errno = 0;
    exitval = semop(atoi(argv[1]), &sops, 1);
    result = exitval < 0;

    fprintf(stderr, "%d %d %d\n", result, result ? errno : exitval, getpid());
    return result;
}
