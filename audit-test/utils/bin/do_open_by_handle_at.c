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

/* search a certain mount id in /proc/self/mountinfo, find corresponding
 * mountpoint and open it, returning opened fd */
int open_mount_id(int mount_id)
{
    FILE *mountinfo;
    int c;
    int read_id;
    char read_point[4096];
    int point_fd;

    mountinfo = fopen("/proc/self/mountinfo", "r");
    if (mountinfo == NULL)
        goto err;

    while (!feof(mountinfo)) {
        if (fscanf(mountinfo,
                   "%d %*u %*u:%*u %*s %4095s", &read_id, read_point) != 2)
            goto err;

        /* line with mount id found */
        if (read_id == mount_id)
            break;

        /* skip to next line */
        do {
            c = fgetc(mountinfo);
        } while (c != '\n' && c != EOF);
    }
    /* mount id not found */
    if (feof(mountinfo))
        goto err;

    point_fd = open(read_point, O_RDONLY);
    if (point_fd == -1)
        goto err;

    fclose(mountinfo);
    return point_fd;
err:
    fclose(mountinfo);
    return -1;
}

int main(int argc, char **argv)
{
    int exitval, result;
    struct file_handle *fhp;
    int dir_fd, mount_id, mount_fd;
    int flags = 0;

    if (argc != 4) {
        fprintf(stderr,
                "Usage:\n%s <directory> <path> <read|write|rdwr>\n",
                argv[0]);
        return TEST_ERROR;
    }

    if (!strcmp(argv[1], "AT_FDCWD"))
        dir_fd = AT_FDCWD;
    else {
        dir_fd = open(argv[1], O_DIRECTORY);
        if (dir_fd == -1) {
            perror("do_open_by_handle_at: open dir_fd");
            return TEST_ERROR;
        }
    }

    /* no "create" as name_to_handle_at needs existing path */
    if (!strcmp(argv[3], "read"))
        flags |= O_RDONLY;
    else if (!strcmp(argv[3], "write"))
        flags |= O_WRONLY;
    else if (!strcmp(argv[3], "rdwr"))
        flags |= O_RDWR;
    else {
        fprintf(stderr, "do_open_by_handle_at: invalid operation\n");
        return TEST_ERROR;
    }

    fhp = malloc(sizeof(struct file_handle) + MAX_HANDLE_SZ);
    if (fhp == NULL) {
        fprintf(stderr, "do_open_by_handle_at: Error on allocating memory\n");
        return TEST_ERROR;
    }

    fhp->handle_bytes = MAX_HANDLE_SZ;
    if (name_to_handle_at(dir_fd, argv[2], fhp, &mount_id, 0) == -1) {
        perror("do_open_by_handle_at: name_to_handle_at");
        return TEST_ERROR;
    }

    mount_fd = open_mount_id(mount_id);
    if (mount_fd == -1) {
        fprintf(stderr, "do_open_by_handle_at: mount id translation failed\n");
        return TEST_ERROR;
    }

    errno = 0;
    exitval = open_by_handle_at(mount_fd, fhp, flags);
    result = exitval < 0;

    fprintf(stderr, "%d %d %d\n", result, result ? errno : exitval, getpid());
    return result;
}
