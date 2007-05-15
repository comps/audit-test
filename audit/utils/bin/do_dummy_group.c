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
    int i, n, pid;

    if (argc < 2) {
	fprintf(stderr, "Usage:\n%s <number of processes>\n", argv[0]);
	return 1;
    }
    n = atoi(argv[1]);

    if (setpgid(0, 0) == -1) {
	perror("do_dummy_group: setpgid");
	return 1;
    }

    for (i = 0; i < n; i++) {

	pid = fork();
	switch (pid) {
	case -1:
	    perror("do_dummy_group: fork");
	    break;
	case 0:
	    for (;;) sleep(1);
	    break;
	}
    }

    for (;;) sleep(1);

    /* not reached */
    return 1;
}
