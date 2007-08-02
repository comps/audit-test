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
#include <grp.h>
#include <sys/param.h> /* for NGROUPS */

int main(int argc, char **argv)
{
    int exitval, result, i;
    gid_t grouplist[NGROUPS] = { 0 };
    size_t nr_groups = 0;

    if (argc < 2) {
	fprintf(stderr, "Usage:\n%s <gid> [gid]...\n", argv[0]);
	return TEST_ERROR;
    }

    nr_groups = getgroups(NGROUPS, grouplist);
    if (nr_groups < 0) {
	perror("do_setgroups: getgroups");
	return TEST_ERROR;
    }

    /* ensure specified groups fit in remaining array slots */
    if (argc - 1 > NGROUPS - nr_groups) {
	fprintf(stderr, "Usage:\n%s you may not specify more than %d supplementary gids)\n",
		argv[0], NGROUPS - nr_groups);
	return TEST_ERROR;
    }

    for (i = 1; i < argc; i++) {
	grouplist[nr_groups] = atoi(argv[i]);
	nr_groups++;
    }

    /* use syscall() to force setgroups32 over setgroups */
    errno = 0;
    exitval = syscall(__NR_setgroups32, nr_groups, &grouplist);
    result = exitval < 0;

    printf("%d %d %d\n", result, result ? errno : exitval, getpid());
    return result;
}
