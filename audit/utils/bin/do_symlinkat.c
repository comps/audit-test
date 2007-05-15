
/*  (c) Copyright Hewlett-Packard Development Company, L.P., 2007
 *
 *  This program is free software;  you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY;  without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See
 *  the GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program;  if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 */

#include "includes.h"
#include <selinux/selinux.h>

int main(int argc, char **argv)
{
    int dir_fd;
    int exitval, result;

    if (argc < 4 || argc > 5) {
	fprintf(stderr, "Usage:\n%s <directory> <oldpath> <newpath> [context]\n", argv[0]);
	return TEST_ERROR;
    }

    dir_fd = open(argv[1], O_DIRECTORY);
    if (dir_fd < 0)
	    return TEST_ERROR;
    if (argc == 5 && setfscreatecon(argv[4]) < 0) {
	perror("do_symlinkat: setfscreatecon");
	return TEST_ERROR;
    }

    errno = 0;
    exitval = symlinkat(argv[2], dir_fd, argv[3]);
    result = exitval < 0;

    printf("%d %d %d\n", result, result ? errno : exitval, getpid());
    return result;
}
