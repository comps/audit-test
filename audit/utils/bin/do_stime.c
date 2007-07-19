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
#include <time.h>

int main(int argc, char **argv)
{
    int exitval, result;
    struct timeval tv;

    if (argc != 2) {
	fprintf(stderr, "Usage:\n%s <seconds>\n", argv[0]);
	return TEST_ERROR;
    }

    memset(&tv, 0, sizeof(tv));
    tv.tv_sec = atol(argv[1]);

    errno = 0;
    exitval = stime(&tv.tv_sec);
    result = exitval < 0;

    printf("%d %d %d\n", result, result ? errno : exitval, getpid());
    return result;
}
