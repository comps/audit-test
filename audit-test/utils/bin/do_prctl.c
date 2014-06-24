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
#include <sys/prctl.h>
#include <linux/capability.h>

int main(int argc, char **argv)
{
    int exitval, result;
    int option, arg2 = CAP_CHOWN;

    if (argc < 2) {
        fprintf(stderr, "Usage:\n%s <option> [arg2]\n", argv[0]);
        return TEST_ERROR;
    }

    if (!strcmp(argv[1], "PR_CAPBSET_READ"))
        option = PR_CAPBSET_READ;
    else if (!strcmp(argv[1], "PR_CAPBSET_DROP"))
        option = PR_CAPBSET_DROP;
    else {
        fprintf(stderr, "Invalid <option> argument\n");
        return TEST_ERROR;
    }

    if (argc == 3) {
        if (!strcmp(argv[2], "CAP_CHOWN"))
            arg2 = CAP_CHOWN;
        else if (!strcmp(argv[2], "CAP_BLOCK_SUSPEND"))
            arg2 = CAP_BLOCK_SUSPEND;
        else if (!strcmp(argv[2], "CAP_IPC_LOCK"))
            arg2 = CAP_IPC_LOCK;
        else if (!strcmp(argv[2], "CAP_NET_ADMIN"))
            arg2 = CAP_NET_ADMIN;
        else if (!strcmp(argv[2], "CAP_SYS_CHROOT"))
            arg2 = CAP_SYS_CHROOT;
        else if (!strcmp(argv[2], "CAP_SYS_ADMIN"))
            arg2 = CAP_SYS_ADMIN;
        else {
            fprintf(stderr, "Invalid [arg2] argument\n");
            return TEST_ERROR;
        }
    }

    errno = 0;
    exitval = prctl(option, arg2, 0, 0, 0);
    result = exitval < 0;

    fprintf(stderr, "%d %d %d\n", result, result ? errno : exitval, getpid());
    return result;
}
