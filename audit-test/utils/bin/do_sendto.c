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
#include "nethelpers.h"

#define MSG_STRING "NetLabel is awesome!"
#define MSG_LEN    (strlen(MSG_STRING) + 1)

int do_sendto(int argc, char **argv,
              ssize_t (*sendtofunc)(int, const void *, size_t, int,
                                    const struct sockaddr *, socklen_t))
{
  int rc, result;
  int sock;
  struct addrinfo *host, hints;
  int flags = 0;
  char *srcport;

  if (argc < 3) {
    fprintf(stderr, "Usage:\n%s <host> <port>[:srcport] [proto] [flags]\n",
            argv[0]);
    return TEST_ERROR;
  }

  memset(&hints, 0, sizeof(hints));
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_DGRAM;
  hints.ai_protocol = IPPROTO_UDP;

  /* proto */
  if (argc >= 4) {
    if (!strcmp(argv[3], "tcp")) {
      hints.ai_socktype = SOCK_STREAM;
      hints.ai_protocol = IPPROTO_TCP;
    }
    /* udp selected by default */
  }

  /* flags */
  if (argc >= 5) {
    if (strstr(argv[4], "MSG_OOB"))
      flags |= MSG_OOB;
  }

  if ((srcport = strchr(argv[2], ':')) != NULL)
    *srcport++ = '\0';

  rc = getaddrinfo(argv[1], argv[2], &hints, &host);
  if (rc < 0)
    return TEST_ERROR;
  sock = socket(host->ai_family, host->ai_socktype, host->ai_protocol);
  if (sock < 0)
    return TEST_ERROR;

  if (srcport)
    if (bind_srcport(sock, atoi(srcport), host->ai_family) == -1)
        return TEST_ERROR;

  set_syn_retries(sock);

  /* connect if stream */
  if (host->ai_socktype == SOCK_STREAM) {
    rc = connect(sock, host->ai_addr, host->ai_addrlen);
    if (rc < 0)
      return TEST_ERROR;
  }

  errno = 0;
  rc = sendtofunc(sock, MSG_STRING, MSG_LEN, flags, host->ai_addr, host->ai_addrlen);
  result = (rc < 0 ? TEST_FAIL : TEST_SUCCESS);

  fprintf(stderr, "%d %d %d\n", result, result ? errno : rc, getpid());
  return result;
}

#ifndef SOCKCALL_MODULE
static ssize_t
sendto_syscall(int sockfd, const void *buf, size_t len, int flags,
               const struct sockaddr *dest_addr, socklen_t addrlen)
{
    return syscall(__NR_sendto, sockfd, buf, len, flags, dest_addr, addrlen);
}

int main(int argc, char **argv)
{
    return do_sendto(argc, argv, sendto_syscall);
}
#endif
