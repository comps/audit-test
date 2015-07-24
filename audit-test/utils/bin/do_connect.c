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

int do_connect(int argc, char **argv,
               int (*connectfunc)(int, const struct sockaddr *, socklen_t))
{
  int rc, result;
  struct addrinfo *host = NULL;
  struct addrinfo addr_hints;
  int sock;
  struct linger so_linger = { .l_onoff = 1, .l_linger = INT_MAX };
  char *srcport;

  if (argc != 4) {
    fprintf(stderr, "Usage:\n%s <host> tcp|udp <port>[:srcport]\n", argv[0]);
    return TEST_ERROR;
  }

  memset(&addr_hints, 0, sizeof(addr_hints));
  if (strcasecmp(argv[2], "tcp") == 0) {
    addr_hints.ai_socktype = SOCK_STREAM;
    addr_hints.ai_protocol = IPPROTO_TCP;
  } else if (strcasecmp(argv[2], "udp") == 0) {
    addr_hints.ai_socktype = SOCK_DGRAM;
    addr_hints.ai_protocol = IPPROTO_UDP;
  } else
    return TEST_ERROR;

  if ((srcport = strchr(argv[3], ':')) != NULL)
    *srcport++ = '\0';

  rc = getaddrinfo(argv[1], argv[3], &addr_hints, &host);
  if (rc < 0)
    return TEST_ERROR;
  sock = socket(host->ai_family, host->ai_socktype, host->ai_protocol);
  if (sock < 0)
    return TEST_ERROR;

  /* bind to srcport if specified */
  if (srcport)
    if (bind_srcport(sock, atoi(srcport), host->ai_family) == -1)
        return TEST_ERROR;

  setsockopt(sock, SOL_SOCKET, SO_LINGER, &so_linger, sizeof(so_linger));

  errno = 0;
  rc = connectfunc(sock, host->ai_addr, host->ai_addrlen);
  result = (rc < 0 ? TEST_FAIL : TEST_SUCCESS);

  fprintf(stderr, "%d %d %d\n", result, result ? errno : rc, getpid());

  shutdown(sock, SHUT_RDWR);
  close(sock);
  return result;
}

#ifndef SOCKCALL_MODULE
static int
connect_syscall(int sockfd, const struct sockaddr *addr, socklen_t addrlen)
{
    return syscall(__NR_connect, sockfd, addr, addrlen);
}

int main(int argc, char **argv)
{
    return do_connect(argc, argv, connect_syscall);
}
#endif
