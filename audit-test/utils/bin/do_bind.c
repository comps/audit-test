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
#include <arpa/inet.h>
#include <linux/un.h>  /* instead of <sys/un.h> for UNIX_PATH_MAX */

int do_bind(int argc, char **argv,
            int (*bindfunc)(int, const struct sockaddr *, socklen_t addrlen))
{
    int exitval, result;
    int sockfd;
    union {
        struct sockaddr_in in;
        struct sockaddr_un un;
    } addr;
    socklen_t addrlen;
    int domain;

    if (argc < 2) {
        fprintf(stderr, "Usage:\n"
                        "%s <address> [port]\n"
                        "%s <unix_path>\n",
                        argv[0], argv[0]);
        return 1;
    }

    /* try ipv4 addr first, fallback to unix domain sockets */
    if (inet_aton(argv[1], NULL)) {
        domain = PF_INET;
        addrlen = sizeof(struct sockaddr_in);
        memset(&addr, 0, addrlen);
        addr.in.sin_family = AF_INET;
        if (argc >= 3)
            addr.in.sin_port = htons(atoi(argv[2]));
        printf("%d\n", ntohs(addr.in.sin_port));
        inet_aton(argv[1], &addr.in.sin_addr);
    } else {
        domain = PF_UNIX;
        addrlen = sizeof(struct sockaddr_un);
        memset(&addr, 0, addrlen);
        addr.un.sun_family = AF_UNIX;
        strncpy(addr.un.sun_path, argv[1], UNIX_PATH_MAX);
    }

    sockfd = socket(domain, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("do_bind: open socket");
        return TEST_ERROR;
    }

    errno = 0;
    exitval = bindfunc(sockfd, (struct sockaddr *)&addr, addrlen);
    result = exitval < 0;

    fprintf(stderr, "%d %d %d\n", result, result ? errno : exitval, getpid());
    return result;
}

#ifndef SOCKCALL_MODULE
int bind_syscall(int sockfd, const struct sockaddr *addr, socklen_t addrlen)
{
    return syscall(__NR_bind, sockfd, addr, addrlen);
}

int main(int argc, char **argv)
{
    return do_bind(argc, argv, bind_syscall);
}
#endif
