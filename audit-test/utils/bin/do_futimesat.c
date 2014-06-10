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
#include <ctype.h>

int main(int argc, char **argv)
{
    struct timeval times[2];
    int exitval, result;
    int dir_fd;

    if (argc < 3) {
        fprintf(stderr, "Usage:\n%s <dir_path> <path> [actime_sec]"
            " [modtime_sec]\n", argv[0]);
        return TEST_ERROR;
    }

    if (!strcmp(argv[1], "AT_FDCWD"))
        dir_fd = AT_FDCWD;
    else {
        dir_fd = open(argv[1], O_DIRECTORY);
        if (dir_fd == -1) {
            perror("do_futimesat: open dir_fd");
            return TEST_ERROR;
        }
    }

    errno = 0;
    if (argc == 5 && isdigit(argv[3][0]) && isdigit(argv[4][0])) {
        times[0].tv_sec = strtoul(argv[3], NULL, 10);
        times[0].tv_usec = 0;
        times[1].tv_sec = strtoul(argv[4], NULL, 10);
        times[1].tv_usec = 0;
        exitval = futimesat(dir_fd, argv[2], times);
    } else {
        /* if times is NULL the access and modification times of the file
           are set to the current time */
        exitval = futimesat(dir_fd, argv[2], NULL);
    }
    result = exitval < 0;

    fprintf(stderr, "%d %d %d\n", result, result ? errno : exitval, getpid());
    return result;
}
