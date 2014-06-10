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
#include <sys/inotify.h>

int main(int argc, char **argv)
{
    int exitval, result;
    int fd;
    uint32_t mask = IN_ACCESS;

    if (argc < 2) {
        fprintf(stderr, "Usage:\n%s <path> [mask]\n"
            "<path> can be also path to a directory\n", argv[0]);
        return TEST_ERROR;
    }

    if (argc == 3) {
        if (!strcmp(argv[2], "IN_ACCESS"))
            mask |= IN_ACCESS;
        else {
            fprintf(stderr, "Invalid [mask] argument\n");
            return TEST_ERROR;
        }
    }

    if ((fd = inotify_init()) < 0) {
        perror("do_inotify_add_watch: inotify_init fd");
        return TEST_ERROR;
    }

    errno = 0;
    exitval = inotify_add_watch(fd, argv[1], mask);
    result = exitval < 0;

    fprintf(stderr, "%d %d %d\n", result, result ? errno : exitval, getpid());
    return result;
}
