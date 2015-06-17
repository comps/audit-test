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

/*
 * this program sets up a buffer of a specified size,
 * prints out address of the buffer to stdout or file specified as "addrout"
 * and waits indefinitely for any terminating signal
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <limits.h>
#include <errno.h>
#include <sys/wait.h>

int main(int argc, char **argv)
{
    int i;
    long size;
    char *endptr;
    void *addr;
    FILE *out = stdout;

    if (argc < 2) {
        printf("usage: vm_transfer_dummy <buffsize> [addrout]\n");
        exit(EXIT_FAILURE);
    }

    errno = 0;
    size = strtoul(argv[1], &endptr, 10);
    if (errno || endptr == argv[1] || size > UINT_MAX) {
        fprintf(stderr, "strtoul failed\n");
        exit(EXIT_FAILURE);
    }

    /* make sure addr points to a valid piece of VM */
    addr = malloc(size);
    if (!addr) {
        perror("malloc");
        exit(EXIT_FAILURE);
    }

    /* output to file */
    if (argc > 2) {
        out = fopen(argv[2], "w");
        if (!out) {
            perror("fopen");
            exit(EXIT_FAILURE);
        }
    }

    fprintf(out, "%p\n", addr);
    fflush(out);

    /* close all FDs - to prevent accidental hangup from tests */
    for (i = 0; i < sysconf(_SC_OPEN_MAX); i++)
        close(i);

    pause();

    return EXIT_SUCCESS;
}

/* vim: set sts=4 sw=4 et : */
