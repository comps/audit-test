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
#include <attr/xattr.h>

int main(int argc, char **argv)
{
    int exitval, result;
    int fd;
    char value[1024];

	/*
        Example, setting the extended attribute:
        $ setfattr -n user.color -v green /path/to/file
        and then:
        $ ./do_fgetxattr /path/to/file "user.color"
    */
    if (argc != 3) {
        fprintf(stderr, "Usage:\n%s <path> <xattr_UTF8string>\n", argv[0]);
        return TEST_ERROR;
    }

    fd = open(argv[1], O_RDWR);
    if (fd == -1) {
        perror("do_fgetxattr: open fd");
        return TEST_ERROR;
    }

    errno = 0;
    exitval = fgetxattr(fd, argv[2], value, sizeof(value));
    result = exitval < 0;

    fprintf(stderr, "%d %d %d\n", result, result ? errno : exitval, getpid());
    close(fd);
    return result;
}
