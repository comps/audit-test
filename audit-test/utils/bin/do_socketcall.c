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

int main(int argc, char **argv)
{
    int exitval, result;
    int sockfd;
    struct sockaddr_in my_addr;
    socklen_t addrlen = sizeof(my_addr);

    if (argc < 2) {
	fprintf(stderr, "Usage:\n%s <op> <arg>...\n", argv[0]);
	return TEST_ERROR;
    }

    if (!strcmp(argv[1], "bind")) {
	if (argc != 4) {
	    fprintf(stderr, "Usage:\n%s bind <port> <0|127>\n", argv[0]);
	    return TEST_ERROR;
	}

	memset(&my_addr, 0, addrlen);
	my_addr.sin_family = AF_INET;
	my_addr.sin_port = htons(atoi(argv[2]));
	my_addr.sin_addr.s_addr = atoi(argv[3]);

	sockfd = socket(PF_INET, SOCK_STREAM, 0);
	if (sockfd < 0) {
	    perror("do_socketcall: open socket");
	    return TEST_ERROR;
	}

	errno = 0;
	/* use library routine for build's sake */
	exitval = bind(sockfd, (struct sockaddr *)&my_addr, addrlen);

    } else {
	fprintf(stderr, "%s: unimplemented op: %s\n", argv[0], argv[1]);
	return TEST_ERROR;
    }

    result = exitval < 0;
    printf("%d %d %d\n", result, result ? errno : exitval, getpid());
    return result;
}
