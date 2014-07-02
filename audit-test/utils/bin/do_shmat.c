/*  (c) Copyright Hewlett-Packard Development Company, L.P., 2006
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
#include <sys/shm.h>

int main(int argc, char **argv)
{
    int result;
    int flags = 0;
    long exitval;

    if (argc != 3) {
        fprintf(stderr, "Usage:\n%s <shmid> <op>\n", argv[0]);
        return 1;
    }

    if (!strcmp(argv[2], "read"))
        flags = SHM_RDONLY;
    else if (!strcmp(argv[2], "write"))
        flags = 0;
    else {
        fprintf(stderr, "op must be one of <read|write>\n");
        return 1;
    }

    errno = 0;
    exitval = (long)shmat(atoi(argv[1]), NULL, flags);
    result = exitval == -1;

    fprintf(stderr, "%d %ld %d\n", result, result ? errno : exitval, getpid());

    if (exitval != -1 && shmdt((void*)exitval) < 0) {
        fprintf(stderr, "Warning: can't detach from shared memory! %s\n",
                strerror(errno));
    }

    return result;
}
