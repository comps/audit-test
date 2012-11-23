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
    int exitval, result;
    struct timeval tv;
    struct timezone tz;

    if (argc < 3) {
	fprintf(stderr, "Usage:\n%s <seconds> <timezone> [DST correction]\n",
		argv[0]);
	return TEST_ERROR;
    }

    memset(&tv, 0, sizeof(tv));
    memset(&tz, 0, sizeof(tz));

    tv.tv_sec = atol(argv[1]);
    tz.tz_minuteswest = atoi(argv[2]);
    if (argc >= 4)
	tz.tz_dsttime = atoi(argv[3]);

    errno = 0;
    exitval = settimeofday(&tv, &tz);
    result = exitval < 0;

    printf("%d %d %d\n", result, result ? errno : exitval, getpid());
    return result;
}
