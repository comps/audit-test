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
#include <sys/mount.h>

int main(int argc, char **argv)
{
    int exitval, result;
    void *data = NULL;
    unsigned long def_flags = MS_MGC_VAL;
    unsigned long flags = def_flags;

    if (argc < 4) {
	fprintf(stderr,
		"Usage:\n%s <source> <target> <type> [data]\n"
		"%s <source> <target> <type> [flags] [data]\n",
		argv[0], argv[0]);
	return 1;
    }

    if (argc > 4) {
        if (strstr(argv[4], "MS_BIND"))
            flags |= MS_BIND;

        /* add more flags here */

        /* if no flags were specified, use the string as data */
        if (flags == def_flags)
            data = argv[4];
    }

    /* if at least one flag matched and an additional arg was
     * specified, use it as data */
    if (argc > 5 && flags != def_flags)
        data = argv[5];

    errno = 0;
    exitval = mount(argv[1], argv[2], argv[3], flags, data);
    result = exitval < 0;

    fprintf(stderr, "%d %d %d\n", result, result ? errno : exitval, getpid());
    return result;
}
