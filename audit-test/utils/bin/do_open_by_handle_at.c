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
    struct file_handle *fhp;
    int fd, dir_fd, mount_id;

    if (argc != 3) {
        fprintf(stderr, "Usage:\n%s <path> <dir_path>\n", argv[0]);
        return TEST_ERROR;
    }

    if (!strcmp(argv[2], "AT_FDCWD"))
        dir_fd = AT_FDCWD;
    else {
        dir_fd = open(argv[2], O_DIRECTORY);
        if (dir_fd == -1) {
            perror("do_open_by_handle_at: open dir_fd");
            return TEST_ERROR;
        }
    }

    fhp = malloc(sizeof(*fhp) + MAX_HANDLE_SZ);
    if (fhp == NULL) {
        fprintf(stderr, "do_open_by_handle_at: Error on allocating memory\n");
        return TEST_ERROR;
    }

    fhp->handle_bytes = MAX_HANDLE_SZ;
    if (name_to_handle_at(dir_fd, argv[1], fhp, &mount_id, 0) == -1) {
        perror("do_open_by_handle_at: name_to_handle_at");
        return TEST_ERROR;
    }

    fd = openat(dir_fd, argv[1], O_RDWR);
    if (fd == -1) {
        perror("do_open_by_handle_at: openat fd");
        return TEST_ERROR;
    }

    errno = 0;
    exitval = open_by_handle_at(fd, fhp, O_RDWR);
    result = exitval < 0;

    fprintf(stderr, "%d %d %d\n", result, result ? errno : exitval, getpid());
    return result;
}
