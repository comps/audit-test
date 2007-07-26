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
	fprintf(stderr, "Usage:\n%s you may not specify more than %zd supplementary gids)\n",
		argv[0], NGROUPS - nr_groups);
	return TEST_ERROR;
    }

    for (i = 1; i < argc; i++) {
	grouplist[nr_groups] = atoi(argv[i]);
	nr_groups++;
    }

    /* use syscall() to force setgroups over setgroups32 */
    errno = 0;
    exitval = syscall(__NR_setgroups, nr_groups, &grouplist);
    result = exitval < 0;

    printf("%d %d %d\n", result, result ? errno : exitval, getpid());
    return result;
}
