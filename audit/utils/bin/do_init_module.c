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
#include <sys/mman.h>

int main(int argc, char **argv)
{
    int exitval, result;
    int fd;
    struct stat mstat;
    void *buffer;

    if (argc != 2) {
	fprintf(stderr, "Usage:\n%s <module_path>\n", argv[0]);
	return 1;
    }

    fd = open(argv[1], O_RDONLY);
    if (fd < 0) {
	perror("do_init_module: open");
	return TEST_ERROR;
    }

    exitval = fstat(fd, &mstat);
    if (exitval < 0) {
	perror("do_init_module: fstat");
	return TEST_ERROR;
    }

    buffer = malloc(mstat.st_size);
    if (!buffer) {
	perror("do_init_module: malloc");
	return TEST_ERROR;
    }

    exitval = read(fd, buffer, mstat.st_size);
    if (exitval < 0) {
	perror("do_init_module: read");
	return TEST_ERROR;
    }

    errno = 0;
    exitval = syscall(__NR_init_module, buffer, mstat.st_size, "");
    result = exitval < 0;

    printf("%d %d %d\n", result, result ? errno : exitval, getpid());
    return result;
}
