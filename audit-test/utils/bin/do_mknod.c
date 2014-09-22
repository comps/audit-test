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
#ifdef LSM_SELINUX
#include <selinux/selinux.h>
#endif

int main(int argc, char **argv)
{
    int exitval, result;

    if (argc < 2) {
	fprintf(stderr, "Usage:\n%s <path> [context]\n", argv[0]);
	return 1;
    }

#ifdef LSM_SELINUX
    if ((argc > 2) && (setfscreatecon(argv[2]) < 0)) {
	perror("do_mknod: setfscreatecon");
	return 1;
    }
#endif

    errno = 0;
    exitval = mknod(argv[1], S_IRWXU, S_IFBLK);
    result = exitval < 0;

    fprintf(stderr, "%d %d %d\n", result, result ? errno : exitval, getpid());
    return result;
}
