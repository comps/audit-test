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

#define BUF_SIZE 2048
#define ALARM_TIMER 15

void sig_handler(int sig_num)
{
  return;
}

int main(int argc, char **argv)
{
  int rc, result;
  struct sockaddr_storage sock_addr;
  struct sockaddr_in *sock_addr4 = (struct sockaddr_in *)&sock_addr;
  struct sockaddr_in6 *sock_addr6 = (struct sockaddr_in6 *)&sock_addr;
  int sock;
  int bool_true = 1;
  char buf[BUF_SIZE];
  size_t buf_len = BUF_SIZE;

  if (argc != 3) {
    fprintf(stderr, "Usage:\n%s ipv4|ipv6 <port>\n", argv[0]);
    return TEST_ERROR;
  }

  signal(SIGALRM, sig_handler);
  siginterrupt(SIGALRM, 1);
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
  sock = socket(sock_addr.ss_family, SOCK_DGRAM, IPPROTO_UDP);
  if (sock < 0)
    return TEST_ERROR;
  rc = setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &bool_true, sizeof(int));
  if (rc < 0)
    return TEST_ERROR;
  rc = bind(sock, (struct sockaddr *)&sock_addr, sizeof(sock_addr));
  if (rc < 0)
    return TEST_ERROR;

  errno = 0;
  rc = recvfrom(sock, buf, buf_len, 0, NULL, NULL);
  result = (rc < 0 ? TEST_FAIL : TEST_SUCCESS);

  fprintf(stderr, "%d %d %d\n", result, result ? errno : rc, getpid());
  return result;
}
