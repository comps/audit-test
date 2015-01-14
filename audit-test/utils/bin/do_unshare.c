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
#include <sched.h>

int main(int argc, char **argv)
{
    int exitval, result;
    int flags = 0;

    if (argc != 2) {
        fprintf(stderr, "Usage:\n%s <flag1,flag2,...>\n", argv[0]);
        return TEST_ERROR;
    }

    /* namespaces */
    if (strstr(argv[1], "CLONE_NEWNS") != NULL)
        flags |= CLONE_NEWNS;
    if (strstr(argv[1], "CLONE_NEWUTS") != NULL)
        flags |= CLONE_NEWUTS;
    if (strstr(argv[1], "CLONE_NEWIPC") != NULL)
        flags |= CLONE_NEWIPC;
    if (strstr(argv[1], "CLONE_NEWNET") != NULL)
        flags |= CLONE_NEWNET;
    if (strstr(argv[1], "CLONE_NEWPID") != NULL)
        flags |= CLONE_NEWPID;

    /* other */
    if (strstr(argv[1], "CLONE_FILES") != NULL)
        flags |= CLONE_FILES;
    if (strstr(argv[1], "CLONE_FS") != NULL)
        flags |= CLONE_FS;
    if (strstr(argv[1], "CLONE_SYSVSEM") != NULL)
        flags |= CLONE_SYSVSEM;

    errno = 0;
    exitval = unshare(flags);
    result = exitval < 0;

    fprintf(stderr, "%d %d %d\n", result, result ? errno : exitval, getpid());
    return result;
}
