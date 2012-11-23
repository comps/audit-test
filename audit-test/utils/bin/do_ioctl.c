/*  (c) Copyright Hewlett-Packard Development Company, L.P., 2007
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
#include <linux/fs.h>
#include <termio.h>
#include <sys/ioctl.h>

int main(int argc, char **argv)
{
    int exitval, result;
    int fd = -1, request;
    struct termios tios = { 0 };
    int map;
    void *arg;

    if (argc != 3) {
	fprintf(stderr, "Usage:\n%s <path> <ioctl op>\n", argv[0]);
	return TEST_ERROR;
    }

    if (!strcmp(argv[2], "TIOCSLCKTRMIOS")) {
	request = TIOCSLCKTRMIOS;
	arg = &tios;
    } else if (!strcmp(argv[2], "FIBMAP")) {
	request = FIBMAP;
	arg = &map;
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
    exitval = ioctl(fd, request, arg);
    result = exitval < 0;

    printf("%d %d %d\n", result, result ? errno : exitval, getpid());
    return result;
}
