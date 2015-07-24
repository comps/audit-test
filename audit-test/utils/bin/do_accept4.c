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
#include <signal.h>

#define ALARM_TIMER 7

#ifndef SIGNAL_DUMMY_HANDLER
#define SIGNAL_DUMMY_HANDLER
void dummy_handler(int signum) { return; }
#endif

int do_accept4(int argc, char **argv,
               int (*accept4func)(int, struct sockaddr *, socklen_t *, int))
{
  int rc, result;
  struct sockaddr_storage sock_addr;
  struct sockaddr_in *sock_addr4 = (struct sockaddr_in *)&sock_addr;
  struct sockaddr_in6 *sock_addr6 = (struct sockaddr_in6 *)&sock_addr;
  int sock;
  int bool_true = 1;

  if (argc < 3 || argc > 4) {
    fprintf(stderr, "Usage:\n%s ipv4|ipv6 <port> <alarmv>\n", argv[0]);
    return TEST_ERROR;
  }

  signal(SIGALRM, dummy_handler);
  siginterrupt(SIGALRM, 1);
  if (argc == 4) {
     alarm(atoi(argv[3]));
  } else
      alarm(ALARM_TIMER);

  memset(&sock_addr, 0, sizeof(sock_addr));
  if (strcasecmp(argv[1], "ipv4") == 0) {
    sock_addr.ss_family = AF_INET;
    sock_addr4->sin_port = htons(atoi(argv[2]));
  } else if (strcasecmp(argv[1], "ipv6") == 0) {
    sock_addr.ss_family = AF_INET6;
    sock_addr6->sin6_port = htons(atoi(argv[2]));
  } else
    return TEST_ERROR;
  sock = socket(sock_addr.ss_family, SOCK_STREAM, IPPROTO_TCP);
  if (sock < 0)
    return TEST_ERROR;
  rc = setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &bool_true, sizeof(int));
  if (rc < 0)
    return TEST_ERROR;
  rc = bind(sock, (struct sockaddr *)&sock_addr, sizeof(sock_addr));
  if (rc < 0)
    return TEST_ERROR;
  rc = listen(sock, 1);
  if (rc < 0)
    return TEST_ERROR;

  errno = 0;
  rc = accept4func(sock, NULL, 0, 0);
  result = (rc < 0 ? TEST_FAIL : TEST_SUCCESS);

  fprintf(stderr, "%d %d %d\n", result, result ? errno : rc, getpid());
  return result;
}

#ifndef SOCKCALL_MODULE
static int
accept4_syscall(int sockfd, struct sockaddr *addr, socklen_t *addrlen,
                int flags)
{
    return syscall(__NR_accept4, sockfd, addr, addrlen, flags);
}

int main(int argc, char **argv)
{
    return do_accept4(argc, argv, accept4_syscall);
}
#endif
