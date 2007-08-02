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

#include <sys/resource.h>

int main(int argc, char **argv)
{
    int exitval, result;
    pid_t pid;


    struct rlimit slimit;

    getrlimit(RLIMIT_NPROC, &slimit);
    fprintf(stderr, "rlim_cur = %d  rlim_max = %d\n",
	    (int)slimit.rlim_cur, (int)slimit.rlim_max);

    /* use syscall() to force fork, as the fork() library routine
     * doesn't call sys_fork on x86_64. */
    errno = 0;
    pid = syscall(__NR_fork);

    /* child */
    if (pid == 0)
	_exit(0);

    /* parent */
    exitval = pid;
    result = exitval < 0;

    printf("%d %d %d\n", result, result ? errno : exitval, getpid());
    return result;
}
