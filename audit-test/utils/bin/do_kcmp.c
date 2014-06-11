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
/* #include <linux/kcmp.h> */


/* from linux/kcmp.h, which is still not part of uapi */
#ifndef _LINUX_KCMP_H
/* Comparison type */
enum kcmp_type {
	KCMP_FILE,
	KCMP_VM,
	KCMP_FILES,
	KCMP_FS,
	KCMP_SIGHAND,
	KCMP_IO,
	KCMP_SYSVSEM,

	KCMP_TYPES,
};
#endif /* _LINUX_KCMP_H */


int main(int argc, char **argv)
{
    int exitval, result;
    pid_t pid1, pid2;
    int type;
    unsigned long fd1 = 0, fd2 = 0;

    if (argc < 4) {
        fprintf(stderr, "Usage:\n%s <pid1> <pid2> <type> [file_path]\n",
            argv[0]);
        return TEST_ERROR;
    }

    pid1 = strtol(argv[1], NULL, 10);
    pid2 = strtol(argv[2], NULL, 10);

    if (!strcmp(argv[3], "KCMP_FILE")) {
        type = KCMP_FILE;
        if (argc == 5) {
            fd1 = open(argv[4], O_RDONLY);
            if (fd1 == -1) {
                perror("do_kcmp: open fd1");
                return TEST_ERROR;
            }
            fd2 = open(argv[4], O_RDWR);
            if (fd2 == -1) {
                perror("do_kcmp: open fd2");
                return TEST_ERROR;
            }
        } else {
            fprintf(stderr, "File path must be specified for KCMP_FILE\n");
            return TEST_ERROR;
        }
    }
    else if (!strcmp(argv[3], "KCMP_FILES"))
        type = KCMP_FILES;
    else if (!strcmp(argv[3], "KCMP_FS"))
        type = KCMP_FS;
    else if (!strcmp(argv[3], "KCMP_IO"))
        type = KCMP_IO;
    else if (!strcmp(argv[3], "KCMP_SIGHAND"))
        type = KCMP_SIGHAND;
    else if (!strcmp(argv[3], "KCMP_SYSVSEM"))
        type = KCMP_SYSVSEM;
    else if (!strcmp(argv[3], "KCMP_VM"))
        type = KCMP_VM;
    else {
        fprintf(stderr, "Invalid <type> argument\n");
        return TEST_ERROR;
    }

    errno = 0;
    exitval = syscall(__NR_kcmp, pid1, pid2, type, fd1, fd2);
    result = exitval < 0;

    fprintf(stderr, "%d %d %d\n", result, result ? errno : exitval, getpid());
    return result;
}
