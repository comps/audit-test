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
#include <pwd.h>

int main(int argc, char **argv)
{
    int exitval, result;
    struct passwd *pw;

    if (argc != 3) {
	fprintf(stderr, "Usage:\n%s <path> <owner>\n", argv[0]);
	return TEST_ERROR;
    }

    pw = getpwnam(argv[2]);
    if (!pw) {
	perror("do_chown: getpwnam");
	return TEST_ERROR;
    }

    /* use syscall() to force chown over chown32 */
    errno = 0;
    exitval = syscall(__NR_chown, argv[1], pw->pw_uid, -1);
    result = exitval < 0;

    printf("%d %d %d\n", result, result ? errno : exitval, getpid());
    return result;
}
