/* Copyright (c) 2014 Red Hat, Inc. All rights reserved.
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
#include <sys/time.h>
#include <sys/resource.h>

int main(int argc, char **argv)
{
    int exitval, result;
    int resource;
    struct rlimit rlim;

    if (argc != 4) {
        fprintf(stderr, "Usage:\n%s <resource> <soft_lim> <hard_lim>\n", argv[0]);
        return TEST_ERROR;
    }

    rlim.rlim_cur = strtoll(argv[2], NULL, 10);
    rlim.rlim_max = strtoll(argv[3], NULL, 10);

    if (!strcmp(argv[1], "RLIMIT_STACK"))
        resource = RLIMIT_STACK;
    else if (!strcmp(argv[1], "RLIMIT_NOFILE"))
        resource = RLIMIT_NOFILE;
    else if (!strcmp(argv[1], "RLIMIT_NPROC"))
        resource = RLIMIT_NPROC;
    else {
        fprintf(stderr, "Invalid <resource> argument\n");
        return TEST_ERROR;
    }

    errno = 0;
    exitval = setrlimit(resource, &rlim);
    result = exitval < 0;

    fprintf(stderr, "%d %d %d\n", result, result ? errno : exitval, getpid());
    return result;
}
