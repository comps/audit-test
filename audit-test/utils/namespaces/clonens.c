/* Copyright (c) 2015 Red Hat, Inc. All rights reserved.
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
 * this program creates a "NS holder" process, a process in the new namespace
 * that serves as PID 1, reaping zombies and keeping the namespace alive
 * - the idea is that, since this process is also reachable from its parent ns,
 *   whenever one wants to destroy the ns, one can do so simply by killing this
 *   process
 *
 * using CLONE_PARENT would allow us to keep the holder process under our
 * existing parent (instead of being moved under init), but unfortunately
 * CLONE_PARENT is incompatible with CLONE_NEWPID
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sched.h>
#include <sys/wait.h>
#include <sys/syscall.h>

int reaper(void)
{
    int i;
    for (i = 0; i < sysconf(_SC_OPEN_MAX); i++)
        close(i);

    while (1) {
        /* collect zombies, wait for next SIGCHLD */
        while (waitpid(-1, NULL, WNOHANG) > 0);
        pause();
    }

    return 0;
}

int main(int argc, char **argv)
{
    int flags, ch;

    if (argc < 2) {
        printf("usage: clonens <type> [type]...\n"
               "       (type is one of mnt,uts,ipc,net,pid,user)\n");
        exit(EXIT_FAILURE);
    }

    for (flags = 0; argc >= 2; argc--) {
        if (!strcmp(argv[argc-1], "mnt")) {
            flags |= CLONE_NEWNS;
        } else if (!strcmp(argv[argc-1], "uts")) {
            flags |= CLONE_NEWUTS;
        } else if (!strcmp(argv[argc-1], "ipc")) {
            flags |= CLONE_NEWIPC;
        } else if (!strcmp(argv[argc-1], "net")) {
            flags |= CLONE_NEWNET;
        } else if (!strcmp(argv[argc-1], "pid")) {
            flags |= CLONE_NEWPID;
        } else if (!strcmp(argv[argc-1], "user")) {
            flags |= CLONE_NEWUSER;
        }
    }
    if (!flags) {
        fprintf(stderr, "no known namespace types matched\n");
        exit(EXIT_FAILURE);
    }

    flags |= SIGCHLD;
    switch (ch = syscall(__NR_clone, flags, NULL, NULL, NULL, NULL)) {
        case -1:
            perror("clone");
            exit(EXIT_FAILURE);
            break;
        case 0:
            return reaper();
        default:
            printf("%d\n", ch);
            break;
    }

    return 0;
}

/* vim: set sts=4 sw=4 et : */
