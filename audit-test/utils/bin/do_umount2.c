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
#include <sys/mount.h>

int main(int argc, char **argv)
{
    int exitval, result;
    int flags = 0;

    if (argc < 2) {
        fprintf(stderr, "Usage:\n%s <target> [flags]\n", argv[0]);
        return TEST_ERROR;
    }

    if (argc > 2) {
        if (strstr(argv[2], "MNT_FORCE"))
            flags |= MNT_FORCE;
        if (strstr(argv[2], "MNT_DETACH"))
            flags |= MNT_DETACH;
        if (strstr(argv[2], "MNT_EXPIRE"))
            flags |= MNT_EXPIRE;
        if (strstr(argv[2], "UMOUNT_NOFOLLOW"))
            flags |= UMOUNT_NOFOLLOW;
    }

    errno = 0;
    exitval = syscall(__NR_umount2, argv[1], flags);
    result = exitval < 0;

    fprintf(stderr, "%d %d %d\n", result, result ? errno : exitval, getpid());
    return result;
}
