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
    int which, who, prio;

    if (argc != 4) {
        fprintf(stderr, "Usage:\n%s <which> <who> <prio>\n", argv[0]);
        return TEST_ERROR;
    }

    if (!strcmp(argv[1], "PRIO_PROCESS"))
        which = PRIO_PROCESS;
    else if (!strcmp(argv[1], "PRIO_PGRP"))
        which = PRIO_PGRP;
    else if (!strcmp(argv[1], "PRIO_USER"))
        which = PRIO_USER;
    else {
        fprintf(stderr, "Invalid <which> argument\n");
        return TEST_ERROR;
    }

    who = strtol(argv[2], NULL, 10);
    prio = strtol(argv[3], NULL, 10);

    errno = 0;
    exitval = setpriority(which, who, prio);
    result = exitval < 0;

    fprintf(stderr, "%d %d %d\n", result, result ? errno : exitval, getpid());
    return result;
}
