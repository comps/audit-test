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

#include <sys/resource.h>

int main(int argc, char **argv)
{
    int exitval, result;
    pid_t pid;


    struct rlimit slimit;

    getrlimit(RLIMIT_NPROC, &slimit);
    fprintf(stderr, "rlim_cur = %d  rlim_max = %d\n", (int)slimit.rlim_cur, (int)slimit.rlim_max);



    /* Must use syscall(), as the fork() library routine doesn't call
     * the fork system call on x86_64. */
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
