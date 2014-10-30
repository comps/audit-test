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
#include <sys/socket.h>
#include <netinet/in.h>
#include <linux/net.h>

#define SOCKCALL_MODULE
#include "do_bind.c"

int bind_socketcall(int sockfd, const struct sockaddr *addr, socklen_t addrlen)
{
    unsigned long arr[] = { sockfd, (unsigned long)addr, addrlen };
    return syscall(__NR_socketcall, SYS_BIND, arr);
}

int main(int argc, char **argv)
{
    char *op;

    if (argc < 2) {
	fprintf(stderr, "Usage:\n%s <op> <arg>...\n", argv[0]);
	return TEST_ERROR;
    }

    op = argv[1];
    argv++;
    argc--;

    if (!strcmp(op, "bind")) {
        return do_bind(argc, argv, bind_socketcall);
    } else {
	fprintf(stderr, "%s: unimplemented op: %s\n", argv[0], argv[1]);
	return TEST_ERROR;
    }
}
