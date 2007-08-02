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

    if (argc < 3) {
	fprintf(stderr, "Usage:\n%s <path> <create|read|write|rdwr|rdcr> [context]\n",
		argv[0]);
	return 1;
    }

    if (!strcmp(argv[2], "create")) {
	/* use O_EXCL on create to catch cleanup problems */
	flags |= O_CREAT|O_EXCL;
    } else if (!strcmp(argv[2], "read")) {
	flags |= O_RDONLY;
    } else if (!strcmp(argv[2], "write")) {
	flags |= O_WRONLY;
    } else if (!strcmp(argv[2], "rdwr")) {
	flags |= O_RDWR;
    } else if (!strcmp(argv[2], "rdcr")) {
	flags |= O_CREAT|O_RDONLY;
    } else {
	fprintf(stderr, "Usage:\n%s <path> <create|read|write|rdwr> [context]\n",
		argv[0]);
	return 1;
    }

    if ((argc > 3) && (setfscreatecon(argv[3]) < 0)) {
	perror("do_open: setfscreatecon");
	return 1;
    }

    errno = 0;
    exitval = open(argv[1], flags, S_IRWXU);
    result = exitval < 0;

    printf("%d %d %d\n", result, result ? errno : exitval, getpid());
    return result;
}
