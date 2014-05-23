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
#include <ctype.h>

int main(int argc, char **argv)
{
    int exitval, result;
    struct timeval times[2];

    if (argc != 2 && argc != 4) {
        fprintf(stderr, "Usage:\n%s <path> [actime_sec] [modtime_sec]\n", argv[0]);
        return TEST_ERROR;
    }

    errno = 0;
    if (argc == 4 && isdigit(argv[2][0]) && isdigit(argv[3][0])) {
        times[0].tv_sec = atol(argv[3]);
        times[0].tv_usec = 0;
        times[1].tv_sec = atol(argv[3]);
        times[1].tv_usec = 0;
        exitval = utimes(argv[1], times);
    } else {
        exitval = utimes(argv[1], NULL);
    }

    result = exitval < 0;

    fprintf(stderr, "%d %d %d\n", result, result ? errno : exitval, getpid());
    return result;
}
