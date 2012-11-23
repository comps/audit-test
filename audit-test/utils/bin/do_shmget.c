/*  (c) Copyright Hewlett-Packard Development Company, L.P., 2006
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

#include "ipc_common.c"

int main(int argc, char **argv)
{
    int exitval, result;
    int flags = 0;

    if (check_ipc_usage("shmget", argc))
	return 1;

    if (translate_ipc_flags(argv[2], &flags))
	return 1;

    errno = 0;
    exitval = do_shmget(atoi(argv[1]), flags);
    result = exitval < 0;

    printf("%d %d %d\n", result, result ? errno : exitval, getpid());
    return result;
}
