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
#include <signal.h>

int main(int argc, char **argv)
{
    int exitval, result;
    int pid, signum;

    if (argc < 3) {
        fprintf(stderr, "Usage:\n%s <pid> <sigkill|sigstop|sigusr1|pidcheck>\n", argv[0]);
        return 1;
    }
    pid = atoi(argv[1]);

    if (!strcmp(argv[2], "sigkill")) {
	signum = SIGKILL;
    } else if (!strcmp(argv[2], "sigstop")) {
	signum = SIGSTOP;
    } else if (!strcmp(argv[2], "sigusr1")) {
	signum = SIGUSR1;
    } else if (!strcmp(argv[2], "pidcheck")) {
       /* "If  sig  is 0, then no signal is sent, but error checking is still
        * performed; this can be used to check for the existence of a process ID
        * or process group ID." */
        signum = 0;
    } else {
        fprintf(stderr, "unsupported signal\n");
        return 1;
    }

    /* use syscall() as no library routine for sys_tkill */
    errno = 0;
    exitval = syscall(__NR_tkill, pid, signum);
    result = exitval < 0;

    fprintf(stderr, "%d %d %d\n", result, result ? errno : exitval, getpid());
    return result;
}
