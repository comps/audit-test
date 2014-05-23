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

int main(int argc, char **argv)
{
    int dir_fd;
    int exitval, result;
    char buf[PATH_MAX];

    if (argc != 3) {
	fprintf(stderr, "Usage:\n%s <directory> <path>\n", argv[0]);
	return TEST_ERROR;
    }

    if (!strcmp(argv[1], "AT_FDCWD")) {
        dir_fd = AT_FDCWD;
    } else {
        dir_fd = open(argv[1], O_DIRECTORY);
        if (dir_fd == -1) {
            perror("do_readlinkat: open dir_fd");
            return TEST_ERROR;
        }
    }

    errno = 0;
    exitval = readlinkat(dir_fd, argv[2], buf, PATH_MAX);
    result = exitval < 0;

    fprintf(stderr, "%d %d %d\n", result, result ? errno : exitval, getpid());
    return result;
}
