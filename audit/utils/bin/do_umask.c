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

int main(int argc, char **argv)
{
    int exitval, result;

    if (argc != 2) {
	fprintf(stderr, "Usage:\n%s <mask>\n", argv[0]);
	return TEST_ERROR;
    }

    errno = 0;
    exitval = umask(atoi(argv[1]));
    result = exitval < 0;

    printf("%d %d %d\n", result, result ? errno : exitval, getpid());
    return result;
}
