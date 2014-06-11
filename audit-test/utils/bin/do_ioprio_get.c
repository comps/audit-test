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


/* from linux/ioprio.h, which is still not part of uapi */
#ifndef IOPRIO_H
enum {
    IOPRIO_WHO_PROCESS = 1,
    IOPRIO_WHO_PGRP,
    IOPRIO_WHO_USER,
};
#endif /* IOPRIO_H */


int main(int argc, char **argv)
{

    /*
     * Usage:
     * ./do_ioprio_get [which] [who]
     */
    int exitval, result;
    int which = IOPRIO_WHO_USER;
    int who = 0;

    if (argc >= 2) {
        if (!strcmp(argv[1], "IOPRIO_WHO_PROCESS"))
            which = IOPRIO_WHO_PROCESS;
        else if (!strcmp(argv[1], "IOPRIO_WHO_PGRP"))
            which = IOPRIO_WHO_PGRP;
        else if (!strcmp(argv[1], "IOPRIO_WHO_USER"))
            which = IOPRIO_WHO_USER;
        else {
            fprintf(stderr, "Invalid [which] argument\n");
            return TEST_ERROR;
        }
    }
    if (argc == 3)
        who = strtol(argv[2], NULL, 10);

    errno = 0;
    exitval = syscall(__NR_ioprio_get, which, who);
    result = exitval < 0;

    fprintf(stderr, "%d %d %d\n", result, result ? errno : exitval, getpid());
    return result;
}
