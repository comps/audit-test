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
#include <sys/uio.h>

int main(int argc, char **argv)
{
    int exitval, result;
    pid_t pid;
    struct iovec local_iov, remote_iov;
    char buf[1];
    unsigned long addr;

    local_iov.iov_base = buf;
    local_iov.iov_len = sizeof(buf);

    if (argc != 3) {
        fprintf(stderr, "Usage:\n%s <pid> <0xaddress>\n"
            "Copies one byte to <0xaddress> of process <pid>\n", argv[0]);
        return TEST_ERROR;
    }
    pid = strtol(argv[1], NULL, 10);
    addr = strtoul(argv[2], NULL, 16);

    remote_iov.iov_base = (void *) addr;
    remote_iov.iov_len = 1;

    errno = 0;
    exitval = process_vm_writev(pid, &local_iov, 1, &remote_iov, 1, 0);
    result = exitval < 0;

    fprintf(stderr, "%d %d %d\n", result, result ? errno : exitval, getpid());
    return result;
}
