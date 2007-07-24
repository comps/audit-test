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
#include <termio.h>
#include <sys/ioctl.h>

int main(int argc, char **argv)
{
    int exitval, result;
    int fd = -1, request;
    struct termios tios = { 0 };

    if (argc != 3) {
	fprintf(stderr, "Usage:\n%s <device file> <ioctl op>\n", argv[0]);
	return TEST_ERROR;
    }

    if (!strcmp(argv[2], "TIOCSLCKTRMIOS")) {
	request = TIOCSLCKTRMIOS;
    } else {
	fprintf(stderr, "%s: unknown request: %s\n", argv[0], argv[2]);
	return TEST_ERROR;
    }

    fd = open(argv[1], O_RDWR, 0777);
    if (fd == -1) {
	perror("do_ioctl: open fd");
	return TEST_ERROR;
    }

    errno = 0;
    exitval = ioctl(fd, request, &tios);
    result = exitval < 0;

    printf("%d %d %d\n", result, result ? errno : exitval, getpid());
    return result;
}
