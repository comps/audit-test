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
    pid_t pid = 0;
    int policy;
    struct sched_param param;

    if (argc != 4) {
        fprintf(stderr, "Usage:\n%s <pid> <policy> <priority>\n"
            "If <pid> is 0, calling process is used\n", argv[0]);
        return TEST_ERROR;
    }

    pid = strtol(argv[1], NULL, 10);

    if (!strcmp(argv[2], "SCHED_OTHER") || !strcmp(argv[2], "SCHED_NORMAL"))
        policy = SCHED_OTHER;
    else if (!strcmp(argv[2], "SCHED_BATCH"))
        policy = SCHED_BATCH;
    else if (!strcmp(argv[2], "SCHED_IDLE"))
        policy = SCHED_IDLE;
    else {
        fprintf(stderr, "Invalid argument <policy>\n");
        return TEST_ERROR;
    }

    memset(&param, 0, sizeof(struct sched_param));
    param.sched_priority = strtol(argv[3], NULL, 10);

    errno = 0;
    exitval = sched_setscheduler(pid, policy, &param);
    result = exitval < 0;

    fprintf(stderr, "%d %d %d\n", result, result ? errno : exitval, getpid());
    return result;
}
