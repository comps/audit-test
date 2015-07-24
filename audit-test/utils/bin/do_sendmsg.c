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

#define MSG_STRING "NetLabel is awesome!"
#define MSG_LEN    (strlen(MSG_STRING) + 1)

#ifndef BIND_SRCPORT
#define BIND_SRCPORT
int bind_srcport(int sock, int port, int family)
{
    int rc;
    union {
        struct sockaddr sa;
        struct sockaddr_in in;
        struct sockaddr_in6 in6;
    } saddr;
    socklen_t slen;

    memset(&saddr, 0, sizeof(saddr));
    saddr.sa.sa_family = family;
    switch (family) {
        case AF_INET:
            slen = sizeof(struct sockaddr_in);
            saddr.in.sin_port = htons(port);
            break;
        case AF_INET6:
            slen = sizeof(struct sockaddr_in6);
            saddr.in6.sin6_port = htons(port);
            break;
        default:
            return -1;
    }

    rc = 1;
    if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &rc, sizeof(rc)) == -1)
        return -1;

    if (bind(sock, &saddr.sa, slen) == -1)
        return -1;

    return sock;
}
#endif

int do_sendmsg(int argc, char **argv,
               ssize_t (*sendmsgfunc)(int, const struct msghdr *, int))
{
  int rc, result;
  struct addrinfo *host = NULL;
  struct addrinfo addr_hints;
  int sock;
  struct msghdr msg;
  struct iovec msg_buf = { .iov_base = MSG_STRING, .iov_len = MSG_LEN };
  char *srcport;

  if (argc != 3) {
    fprintf(stderr, "Usage:\n%s <host> <port>[:srcport]\n", argv[0]);
    return TEST_ERROR;
  }

  memset(&msg, 0, sizeof(msg));
  memset(&addr_hints, 0, sizeof(addr_hints));
  addr_hints.ai_socktype = SOCK_DGRAM;
  addr_hints.ai_protocol = IPPROTO_UDP;

  if ((srcport = strchr(argv[2], ':')) != NULL)
    *srcport++ = '\0';

  rc = getaddrinfo(argv[1], argv[2], &addr_hints, &host);
  if (rc < 0)
    return TEST_ERROR;
  sock = socket(host->ai_family, host->ai_socktype, host->ai_protocol);
  if (sock < 0)
    return TEST_ERROR;
  msg.msg_name = host->ai_addr;
  msg.msg_namelen = host->ai_addrlen;
  msg.msg_iov = &msg_buf;
  msg.msg_iovlen = 1;

  /* bind to srcport if specified */
  if (srcport)
    if (bind_srcport(sock, atoi(srcport), host->ai_family) == -1)
      return TEST_ERROR;

  errno = 0;
  rc = sendmsgfunc(sock, &msg, 0);
  result = (rc < 0 ? TEST_FAIL : TEST_SUCCESS);

  fprintf(stderr, "%d %d %d\n", result, result ? errno : rc, getpid());
  return result;
}

#ifndef SOCKCALL_MODULE
static ssize_t
sendmsg_syscall(int sockfd, const struct msghdr *msg, int flags)
{
    return syscall(__NR_sendmsg, sockfd, msg, flags);
}

int main(int argc, char **argv)
{
    return do_sendmsg(argc, argv, sendmsg_syscall);
}
#endif
