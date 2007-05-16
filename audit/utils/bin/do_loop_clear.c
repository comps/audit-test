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
#include <linux/loop.h>
#include <sys/ioctl.h>

int main(int argc, char **argv)
{
    int loop_fd;

    if (argc != 2) {
	fprintf(stderr, "Usage:\n%s <loop device>\n", argv[0]);
	return TEST_ERROR;
    }

    loop_fd = open(argv[1], O_RDWR);
    if (loop_fd == -1) {
	perror("do_loop: open loop_fd");
	return TEST_ERROR;
    }

    if (ioctl(loop_fd, LOOP_CLR_FD) == -1) {
	perror("do_loop: ioctl loop_clr_fd");
	return TEST_ERROR;
    }

    close(loop_fd);

    return 0;
}
