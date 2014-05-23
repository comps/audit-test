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
#include <sched.h>

int main(int argc, char **argv)
{
    int exitval, result;
    int flags = CLONE_VFORK;
    pid_t pid;

    /* Usage: "%s [flag]\n", argv[0] */

    if (argc >= 2) {
        if (!strcmp(argv[1], "newns")) {
            flags |= CLONE_NEWNS;
        } else {
            fprintf(stderr, "unknown flag: %s\n", argv[1]);
            return TEST_ERROR;
        }
    }

    /* use syscall() to force clone over clone2 */
    errno = 0;
    /* On s390x, the param order is different for clone */
#if defined (S390X)
    pid = syscall(__NR_clone, 0, flags);
#else
    pid = syscall(__NR_clone, flags, 0);
#endif

    /* child */
    if (pid == 0)
	_exit(0);

    /* parent */
    exitval = pid;
    result = exitval < 0;

    fprintf(stderr, "%d %d %d\n", result, result ? errno : exitval, getpid());
    return result;
}
