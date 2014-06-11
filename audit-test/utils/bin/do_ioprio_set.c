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

#define IOPRIO_CLASS_SHIFT      (13)
#define IOPRIO_PRIO_VALUE(class, data)  (((class) << IOPRIO_CLASS_SHIFT) | data)

enum {    /* scheduling class */
    IOPRIO_CLASS_NONE,
    IOPRIO_CLASS_RT,
    IOPRIO_CLASS_BE,
    IOPRIO_CLASS_IDLE,
};

enum {    /* which */
    IOPRIO_WHO_PROCESS = 1,
    IOPRIO_WHO_PGRP,
    IOPRIO_WHO_USER,
};

#endif /* IOPRIO_H */


int main(int argc, char **argv)
{
    int exitval, result;
    int which = IOPRIO_WHO_USER;
    int who = 0;
    int ioprio;
    int sched_class;
    int priority_level = 0;

    if (argc < 3) {
        fprintf(stderr, "Usage:\n%s <sched_class> <priority_level>"
            " [which] [who]\n", argv[0]);
        return TEST_ERROR;
    }

    if (!strcmp(argv[1], "IOPRIO_CLASS_RT"))
        sched_class = IOPRIO_CLASS_RT;
    else if (!strcmp(argv[1], "IOPRIO_CLASS_BE"))
        sched_class = IOPRIO_CLASS_BE;
    else if (!strcmp(argv[1], "IOPRIO_CLASS_IDLE"))
        sched_class = IOPRIO_CLASS_IDLE;
    else {
        fprintf(stderr, "Invalid <sched_class> argument\n");
        return TEST_ERROR;
    }
    priority_level = strtol(argv[2], NULL, 10);

    if (argc >= 4) {
        if (!strcmp(argv[3], "IOPRIO_WHO_PROCESS"))
            which = IOPRIO_WHO_PROCESS;
        else if (!strcmp(argv[3], "IOPRIO_WHO_PGRP"))
            which = IOPRIO_WHO_PGRP;
        else if (!strcmp(argv[3], "IOPRIO_WHO_USER"))
            which = IOPRIO_WHO_USER;
        else {
            fprintf(stderr, "Invalid [which] argument\n");
            return TEST_ERROR;
        }
    }
    if (argc == 5)
        who = strtol(argv[4], NULL, 10);

    errno = 0;
    ioprio = IOPRIO_PRIO_VALUE(sched_class, priority_level);
    exitval = syscall(__NR_ioprio_set, which, who, ioprio);
    result = exitval < 0;

    fprintf(stderr, "%d %d %d\n", result, result ? errno : exitval, getpid());
    return result;
}
