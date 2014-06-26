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

/* from linux kernel source, include/syslog.h */
#ifndef _LINUX_SYSLOG_H
#define _LINUX_SYSLOG_H

/* Close the log.  Currently a NOP. */
#define SYSLOG_ACTION_CLOSE          0
/* Open the log. Currently a NOP. */
#define SYSLOG_ACTION_OPEN           1
/* Read from the log. */
#define SYSLOG_ACTION_READ           2
/* Read all messages remaining in the ring buffer. */
#define SYSLOG_ACTION_READ_ALL       3
/* Read and clear all messages remaining in the ring buffer */
#define SYSLOG_ACTION_READ_CLEAR     4
/* Clear ring buffer. */
#define SYSLOG_ACTION_CLEAR          5
/* Disable printk's to console */
#define SYSLOG_ACTION_CONSOLE_OFF    6
/* Enable printk's to console */
#define SYSLOG_ACTION_CONSOLE_ON     7
/* Set level of messages printed to console */
#define SYSLOG_ACTION_CONSOLE_LEVEL  8
/* Return number of unread characters in the log buffer */
#define SYSLOG_ACTION_SIZE_UNREAD    9
/* Return size of the log buffer */
#define SYSLOG_ACTION_SIZE_BUFFER   10

#define SYSLOG_FROM_READER           0
#define SYSLOG_FROM_PROC             1

#endif /* _LINUX_SYSLOG_H */


int main(int argc, char **argv)
{
    int exitval, result;
    int type, len;
    char *bufp = NULL;

    if (argc != 3) {
        fprintf(stderr, "Usage:\n%s <type> <len>\n", argv[0]);
        return TEST_ERROR;
    }

    len = strtol(argv[2], NULL, 10);

    if (!strcmp(argv[1], "SYSLOG_ACTION_READ")) {
        type = SYSLOG_ACTION_READ;
        bufp = malloc(len * sizeof(char));
        if (bufp == NULL) {
            fprintf(stderr, "Error on allocating memory for bufp\n");
            return TEST_ERROR;
        }
    } else if (!strcmp(argv[1], "SYSLOG_ACTION_CONSOLE_LEVEL")) {
        type = SYSLOG_ACTION_CONSOLE_LEVEL;
    } else {
        fprintf(stderr, "Invalid <type> argument\n");
        return TEST_ERROR;
    }

    errno = 0;
    exitval = syscall(__NR_syslog, type, bufp, len);
    result = exitval < 0;

    fprintf(stderr, "%d %d %d\n", result, result ? errno : exitval, getpid());
    return result;
}
