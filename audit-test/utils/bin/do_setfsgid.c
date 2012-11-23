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
    int exitval, result;

    if (argc != 2) {
	fprintf(stderr, "Usage:\n%s <fsgid>\n", argv[0]);
	return TEST_ERROR;
    }

    /* use syscall() to force setfsgid over setfsgid32 */
    errno = 0;
    exitval = syscall(__NR_setfsgid, atoi(argv[1]));
    result = exitval < 0;

    printf("%d %d %d\n", result, result ? errno : exitval, getpid());
    return result;
}
