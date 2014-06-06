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

int main(int argc, char **argv)
{
    int exitval, result;
    int flags = 0;
    struct stat buf;
    int dir_fd;

    if (argc < 3) {
        fprintf(stderr, "Usage:\n%s <directory> <path> [flags]\n", argv[0]);
        return TEST_ERROR;
    }

    if (!strcmp(argv[1], "AT_FDCWD"))
        dir_fd = AT_FDCWD;
    else {
        dir_fd = open(argv[1], O_DIRECTORY);
        if (dir_fd == -1) {
            perror("do_fstatat: open dir_fd");
            return TEST_ERROR;
        }
    }

    if (argc == 4) {
        if (!strcmp(argv[3], "AT_EMPTY_PATH"))
            flags |= AT_EMPTY_PATH;
        if (!strcmp(argv[3], "AT_NO_AUTOMOUNT"))
            flags |= AT_NO_AUTOMOUNT;
        if (!strcmp(argv[3], "AT_SYMLINK_NOFOLLOW"))
            flags |= AT_SYMLINK_NOFOLLOW;
        else {
            fprintf(stderr, "Invalid [flags] argument\n");
            return TEST_ERROR;
        }
    }

    errno = 0;
    exitval = syscall(__NR_fstatat64, dir_fd, argv[2], &buf, flags);
    result = exitval < 0;

    fprintf(stderr, "%d %d %d\n", result, result ? errno : exitval, getpid());
    return result;
}
