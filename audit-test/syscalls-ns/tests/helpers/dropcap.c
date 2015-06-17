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
 * this simple utility can be used to drop capabilities (as root) in order
 * to perform regular-user-like tasks still under root
 *
 * the --drop option of capsh is somehow horribly broken, hence this tool
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/capability.h>

int main(int argc, char **argv)
{
    cap_value_t drop;
    char *capstr;

    if (argc < 3) {
        printf("usage: capdrop <capname>[,capname] <cmd> [args]\n");
        exit(EXIT_FAILURE);
    }

    capstr = strtok(argv[1], ",");
    while (capstr != NULL) {
        if (cap_from_name(capstr, &drop) == -1) {
            fprintf(stderr, "cap_from_name failed\n");
            exit(EXIT_FAILURE);
        }

        if (cap_drop_bound(drop) == -1) {
            fprintf(stderr, "cap_drop_bound failed\n");
            exit(EXIT_FAILURE);
        }

        capstr = strtok(NULL, ",");
    }

    if (execvp(argv[2], argv+2) == -1) {
        perror("execvp");
        exit(EXIT_FAILURE);
    }

    /* execve shouldn't return */
    return EXIT_FAILURE;
}

/* vim: set sts=4 sw=4 et : */
