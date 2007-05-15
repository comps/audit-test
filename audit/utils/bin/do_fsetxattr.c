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
#include <sys/xattr.h>

int main(int argc, char **argv)
{
    int exitval, result;
    int fd;

    if (argc != 4) {
	fprintf(stderr, "Usage:\n%s <path> <xattr name> <xattr value>\n",
		argv[0]);
	return 1;
    }

    fd = open(argv[1], O_RDWR);
    if (fd == -1) {
	perror("do_fsetxattr: open fd");
	return TEST_ERROR;
    }

    errno = 0;
    exitval = fsetxattr(fd, argv[2], argv[3], strlen(argv[3]), XATTR_CREATE);
    result = exitval < 0;

    printf("%d %d %d\n", result, result ? errno : exitval, getpid());
    return result;
}
