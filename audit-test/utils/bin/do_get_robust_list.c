/* Copyright (c) 2015 Red Hat, Inc. All rights reserved.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of version 2 the GNU General Public License as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "includes.h"

int main(int argc, char **argv)
{
    int exitval, result;

    int pid;
    struct robust_list_head *listhead;
    size_t listlen;

    if (argc != 2) {
        fprintf(stderr, "Usage:\n%s <pid>\n", argv[0]);
        return TEST_ERROR;
    }

    pid = strtol(argv[1], NULL, 10);

    errno = 0;
    exitval = syscall(__NR_get_robust_list, pid, &listhead, &listlen);
    result = exitval < 0;

    fprintf(stderr, "%d %d %d\n", result, result ? errno : exitval, getpid());
    return result;
}

/* vim: set sts=4 sw=4 et : */
