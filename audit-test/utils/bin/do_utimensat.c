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
#include <sys/time.h>

int main(int argc, char **argv)
{
    struct timespec times[2];
    int exitval, result = 0;
    int dirfd;

    if (argc != 3) {
	fprintf(stderr, "Usage:\n%s <dir> <path>\n", argv[0]);
	return TEST_ERROR;
    }

    dirfd = open(argv[1], O_DIRECTORY);
    if (dirfd == -1) {
        perror("do_utimensat: open dirfd");
        return TEST_ERROR;
    }

    times[0].tv_sec = 0;
    times[0].tv_nsec = UTIME_NOW;
    times[1].tv_sec = 0;
    times[1].tv_nsec = UTIME_NOW;

    errno = 0;
    exitval = utimensat(dirfd, argv[2], times, 0);
    result = exitval < 0;

    printf("%d %d %d\n", result, result ? errno : exitval, getpid());
    return result;
}
