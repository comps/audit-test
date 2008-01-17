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

    if (argc != 2) {
	fprintf(stderr, "Usage:\n%s <flag>\n", argv[0]);
	return TEST_ERROR;
    }

    if (!strcmp(argv[1], "newns")) {
	flags |= CLONE_NEWNS;
    } else {
	fprintf(stderr, "Usage:\n%s: unknown flag: %s\n", argv[0], argv[1]);
	return TEST_ERROR;
    }

    /* use syscall() to force clone over clone2 */
    errno = 0;
    pid = syscall(__NR_clone, flags, 0);

    /* child */
    if (pid == 0)
	_exit(0);

    /* parent */
    exitval = pid;
    result = exitval < 0;

    printf("%d %d %d\n", result, result ? errno : exitval, getpid());
    return result;
}
