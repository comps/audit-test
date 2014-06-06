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
#include <sys/fanotify.h>

int main(int argc, char **argv)
{
    int exitval, result;
    int fan_fd;
    unsigned int flags = FAN_MARK_ADD;
    uint64_t mask = FAN_OPEN;

    if (argc < 2) {
        fprintf(stderr, "Usage:\n%s <path> [mask] [flags]\n", argv[0]);
        return TEST_ERROR;
    }

    if (argc >= 3) {
        mask = 0;
        if (!strcmp(argv[2], "FAN_OPEN"))
            mask |= FAN_OPEN;
        else
            return TEST_ERROR;
    }
    if (argc >= 4) {
        flags = 0;
        if (!strcmp(argv[3], "FAN_MARK_ADD"))
            flags |= FAN_MARK_ADD;
        else
            return TEST_ERROR;
    }

    fan_fd = fanotify_init(FAN_CLASS_NOTIF, O_RDWR);
    if (fan_fd == -1) {
        perror("do_fanotify_mark: fanotify_init fan_fd");
        return TEST_ERROR;
    }
    errno = 0;
    exitval = fanotify_mark(fan_fd, flags, mask, AT_FDCWD, argv[1]);
    result = exitval < 0;

    fprintf(stderr, "%d %d %d\n", result, result ? errno : exitval, getpid());
    return result;
}
