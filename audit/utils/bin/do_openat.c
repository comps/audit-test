/*  (c) Copyright Hewlett-Packard Development Company, L.P., 2006
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of version 2 the GNU General Public License as
 *  published by the Free Software Foundation.
 *  
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *  
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "includes.h"
#include <selinux/selinux.h>

int main(int argc, char **argv)
{
    int exitval, result;
    int flags = 0;
    int dirfd;

    if (argc < 4 || argc > 5) {
	fprintf(stderr,
		"Usage:\n%s <directory> <filename> <create|read|write|rdwr>"
		" [context]\n",
		argv[0]);
	return TEST_ERROR;
    }

    if (!strcmp(argv[3], "create")) {
	/* use O_EXCL on create to catch cleanup problems */
	flags |= O_CREAT|O_EXCL;
    } else if (!strcmp(argv[3], "read")) {
	flags |= O_RDONLY;
    } else if (!strcmp(argv[3], "write")) {
	flags |= O_WRONLY;
    } else if (!strcmp(argv[3], "rdwr")) {
	flags |= O_RDWR;
    } else {
	fprintf(stderr,
		"Usage:\n%s <directory> <filename> <create|read|write|rdwr>\n",
		argv[0]);
	return TEST_ERROR;
    }

    dirfd = open(argv[1], O_DIRECTORY);
    if (dirfd == -1) {
	perror("do_openat: open dirfd");
	return TEST_ERROR;
    }
    if (argc == 5 && setfscreatecon(argv[4]) < 0) {
	perror("do_openat: setfscreatecon");
	return TEST_ERROR;
    }

    errno = 0;
    exitval = openat(dirfd, argv[2], flags);
    result = exitval < 0;

    printf("%d %d %d\n", result, result ? errno : exitval, getpid());
    return result;
}
