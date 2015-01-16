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

/* for some do_* syscall wrappers */
void dummy_handler(int signum) { return; }

#define SOCKCALL_MODULE
#include "do_accept.c"
#include "do_accept4.c"
#include "do_bind.c"
#include "do_connect.c"
#include "do_recvfrom.c"
#include "do_recvmsg.c"
#include "do_sendmsg.c"
#include "do_sendto.c"

static int
accept_socketcall(int sockfd, struct sockaddr *addr, socklen_t *addrlen)
{
    unsigned long arr[] = { sockfd, (unsigned long)addr,
                            (unsigned long)addrlen };
    return syscall(__NR_socketcall, SYS_ACCEPT, arr);
}

static int
accept4_socketcall(int sockfd, struct sockaddr *addr, socklen_t *addrlen,
                   int flags)
{
    unsigned long arr[] = { sockfd, (unsigned long)addr,
                            (unsigned long)addrlen, flags };
    return syscall(__NR_socketcall, SYS_ACCEPT4, arr);
}

static int
bind_socketcall(int sockfd, const struct sockaddr *addr, socklen_t addrlen)
{
    unsigned long arr[] = { sockfd, (unsigned long)addr, addrlen };
    return syscall(__NR_socketcall, SYS_BIND, arr);
}

static int
connect_socketcall(int sockfd, const struct sockaddr *addr,
                   socklen_t addrlen)
{
    unsigned long arr[] = { sockfd, (unsigned long)addr, addrlen };
    return syscall(__NR_socketcall, SYS_CONNECT, arr);
}

static ssize_t
recvfrom_socketcall(int sockfd, void *buf, size_t len, int flags,
                    struct sockaddr *src_addr, socklen_t *addrlen)
{
    unsigned long arr[] = { sockfd, (unsigned long)buf, len, flags,
                            (unsigned long)src_addr, (unsigned long)addrlen };
    return syscall(__NR_socketcall, SYS_RECVFROM, arr);
}

static ssize_t
recvmsg_socketcall(int sockfd, struct msghdr *msg, int flags)
{
    unsigned long arr[] = { sockfd, (unsigned long)msg, flags };
    return syscall(__NR_socketcall, SYS_RECVMSG, arr);
}

static ssize_t
sendmsg_socketcall(int sockfd, const struct msghdr *msg, int flags)
{
    unsigned long arr[] = { sockfd, (unsigned long)msg, flags };
    return syscall(__NR_socketcall, SYS_SENDMSG, arr);
}

static ssize_t
sendto_socketcall(int sockfd, const void *buf, size_t len, int flags,
                  const struct sockaddr *dest_addr, socklen_t addrlen)
{
    unsigned long arr[] = { sockfd, (unsigned long)buf, len, flags,
                            (unsigned long)dest_addr, addrlen };
    return syscall(__NR_socketcall, SYS_SENDTO, arr);
}


int main(int argc, char **argv)
{
    char *thiscmd, *op;

    if (argc < 2) {
	fprintf(stderr, "Usage:\n%s <op> <arg>...\n", argv[0]);
	return TEST_ERROR;
    }

    thiscmd = argv[0];
    op = argv[1];
    argv++;
    argc--;

    if (!strcmp(op, "accept"))
        return do_accept(argc, argv, accept_socketcall);
    else if (!strcmp(op, "accept4"))
        return do_accept4(argc, argv, accept4_socketcall);
    else if (!strcmp(op, "bind"))
        return do_bind(argc, argv, bind_socketcall);
    else if (!strcmp(op, "connect"))
        return do_connect(argc, argv, connect_socketcall);
    else if (!strcmp(op, "recvfrom"))
        return do_recvfrom(argc, argv, recvfrom_socketcall);
    else if (!strcmp(op, "recvmsg"))
        return do_recvmsg(argc, argv, recvmsg_socketcall);
    else if (!strcmp(op, "sendmsg"))
        return do_sendmsg(argc, argv, sendmsg_socketcall);
    else if (!strcmp(op, "sendto"))
        return do_sendto(argc, argv, sendto_socketcall);
    else {
	fprintf(stderr, "%s: unimplemented op: %s\n", thiscmd, op);
	return TEST_ENOSYS;
    }
}
