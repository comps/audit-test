/*  (c) Copyright Hewlett-Packard Development Company, L.P., 2007
 *
 *  This program is free software;  you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY;  without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See
 *  the GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program;  if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 */

#include "includes.h"
#include <sched.h>

#define CHILD_STACK_MEM 65536

int main(int argc, char **argv)
{
    int exitval, result;
    int flags = CLONE_VFORK;
    char *cstack;
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

    cstack = malloc(CHILD_STACK_MEM); 
    if (!cstack) {
	perror("do_clone: malloc");
	return TEST_ERROR;
    }

    /* use syscall() to force clone over clone2 */
    errno = 0;
    pid = syscall(__NR_clone, flags, cstack);

    /* child */
    if (pid == 0)
	_exit(0);

    /* parent */
    free(cstack);
    exitval = pid;
    result = exitval < 0;

    printf("%d %d %d\n", result, result ? errno : exitval, getpid());
    return result;
}
