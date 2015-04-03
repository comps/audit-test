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
 * this program serves as a pid 0 "init wrapper" for namespace testing,
 * a task that would be normally fulfilled by the "idle task" (swapper/sched/..)
 * in the Linux kernel
 *
 * this program should be execve'd by something that does unshare(2)
 * or setns(2), so that it retains its original PID in the old namespace,
 * while being able to fork() a process with PID 1 in the new namespace
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

int main(int argc, char **argv)
{
    int status;

    if (argc < 2) {
        printf("usage: initwrap <cmd> [args]\n");
        exit(EXIT_FAILURE);
    }

    switch (fork()) {
        case -1:
            perror("fork");
            exit(EXIT_FAILURE);
            break;
        case 0:
            if (execvp(argv[1], argv+1) == -1) {
                perror("execvp");
                exit(EXIT_FAILURE);
            }
            break;
        default:
            break;
    }

    if (wait(&status) == -1) {
        perror("wait");
        exit(EXIT_FAILURE);
    }

    return WEXITSTATUS(status);
}

/* vim: set sts=4 sw=4 et : */
