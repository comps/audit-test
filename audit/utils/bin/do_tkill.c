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
#include <signal.h>

int main(int argc, char **argv)
{
    int exitval, result;
    int pid, signum;

    if (argc < 3) {
	fprintf(stderr, "Usage:\n%s <pid> <sigkill|sigstop|sigusr1>\n", argv[0]);
	return 1;
    }
    pid = atoi(argv[1]);

    if (!strcmp(argv[2], "sigkill")) {
	signum = SIGKILL;
    } else if (!strcmp(argv[2], "sigstop")) {
	signum = SIGSTOP;
    } else if (!strcmp(argv[2], "sigusr1")) {
	signum = SIGUSR1;
    } else {
	fprintf(stderr, "Usage:\n%s <pid> <sigkill|sigstop|sigusr1>\n", argv[0]);
	return 1;
    }

    /* use syscall() as no library routine for sys_tkill */
    errno = 0;
    exitval = syscall(__NR_tkill, pid, signum);
    result = exitval < 0;

    printf("%d %d %d\n", result, result ? errno : exitval, getpid());
    return result;
}
