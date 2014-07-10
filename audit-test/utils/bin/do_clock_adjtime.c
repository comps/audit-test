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
#include <time.h>
#include <sys/timex.h>

int main(int argc, char **argv)
{
    int exitval, result;
    struct timex tx;
    memset(&tx, 0, sizeof(tx));

    if (argc != 2) {
        fprintf(stderr,
                "Usage:\n%s <offset>\n<offset> is time offset (usec)\n",
                argv[0]);
        return TEST_ERROR;
    }
    tx.modes = ADJ_OFFSET;
    tx.offset = strtol(argv[1], NULL, 10);

    errno = 0;
    exitval = syscall(__NR_clock_adjtime, CLOCK_REALTIME, &tx);
    result = exitval < 0;

    fprintf(stderr, "%d %d %d\n", result, result ? errno : exitval, getpid());
    return result;
}
