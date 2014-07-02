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
    int exitval, result;
    int flags = 0;

    if (argc != 3) {
        fprintf(stderr, "Usage:\n%s <ipc_key> <flag>\n", argv[0]);
        return 1;
    }

    if (!strcmp(argv[2], "create")) {
        /* use IPC_EXCL on create to catch cleanup problems */
        flags |= IPC_CREAT|IPC_EXCL;
        /* create with default mode */
        flags |= S_IRUSR|S_IWUSR;
    } else if (!strncmp(argv[2], "create:", 7)) {
        flags |= IPC_CREAT|IPC_EXCL;
        /* create with custom mode */
        flags |= strtol(argv[2]+7, NULL, 8);
    }
    else if (!strcmp(argv[2], "read"))
        flags |= S_IRUSR;
    else if (!strcmp(argv[2], "write"))
        flags |= S_IWUSR;
    else if (!strcmp(argv[2], "rdwr"))
        flags |= S_IRUSR|S_IWUSR;
    else
        flags |= atoi(argv[2]);

    errno = 0;
    exitval = shmget(atoi(argv[1]), sysconf(_SC_PAGESIZE), flags);
    result = exitval < 0;

    if (result == 0)
        printf("%d\n", exitval);

    fprintf(stderr, "%d %d %d\n", result, result ? errno : exitval, getpid());
    return result;
}
