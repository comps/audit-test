/* Copyright (c) 2013 Red Hat, Inc. All rights reserved.
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

/* AUTHOR: Jiri Jaburek <jjaburek@redhat.com>
 *
 * This tool processes a list of pidfiles passed on cmdline,
 * extracts PIDs from them an issues SIGKILL to those PIDs.
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/types.h>

int main(int argc, char **argv)
{
    int fd;
    pid_t pid;
    char pidstr[24] = {0};  /* for extremely large pid_max */

    for (;--argc;argv++) {
        /* check if pidfile exists */
        if (access(argv[1], F_OK) == -1)
            continue;

        /* read pid from file */
        if ((fd = open(argv[1], O_RDONLY)) == -1)
            continue;
        read(fd, pidstr, sizeof(pidstr)-1);
        close(fd);

        /* get numeric pid */
        pid = atoi(pidstr);
        if (pid == 0)
            continue;

        /* signal the process, ignore return value
         * (the process might not exist anymore) */
        kill(pid, SIGKILL);
    }

    return 0;
}
