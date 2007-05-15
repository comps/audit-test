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
#include <sys/ptrace.h>

int main(int argc, char **argv)
{
    int exitval, result;
    enum __ptrace_request req;

    if (argc < 3) {
	fprintf(stderr, "Usage:\n%s <pid> <attach|detach|kill>\n", argv[0]);
	return 1;
    }

    if (!strcmp(argv[2], "attach")) {
	req = PTRACE_ATTACH;
    } else if (!strcmp(argv[2], "detach")) {
	req = PTRACE_DETACH;
    } else if (!strcmp(argv[2], "kill")) {
	req = PTRACE_KILL;
    } else {
	fprintf(stderr, "Usage:\n%s <pid> <attach|detach|kill>\n", argv[0]);
	return 1;
    }

    errno = 0;
    exitval = ptrace(req, atoi(argv[1]), NULL, NULL);
    result = exitval < 0;

    printf("%d %d %d\n", result, result ? errno : exitval, getpid());
    return result;
}
