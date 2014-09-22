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

#define MSG_STRING "Message to run some data over a tcp connection with URG bit set!"
#define MSG_LEN    (strlen(MSG_STRING) + 1)

int main(int argc, char **argv)
{
  int rc, result, sflgs = 0;
  struct addrinfo *host = NULL;
  struct addrinfo addr_hints;
  int sock;

  if (argc < 4) {
    fprintf(stderr, "Usage:\n%s <host> tcp|udp <port>\n", argv[0]);
    return TEST_ERROR;
  }

  memset(&addr_hints, 0, sizeof(addr_hints));
  if (strcasecmp(argv[2], "tcp") == 0) {
    addr_hints.ai_socktype = SOCK_STREAM;
    addr_hints.ai_protocol = IPPROTO_TCP;
    sflgs |= MSG_OOB;
  } else if (strcasecmp(argv[2], "udp") == 0) {
    addr_hints.ai_socktype = SOCK_DGRAM;
    addr_hints.ai_protocol = IPPROTO_UDP;
  } else
    return TEST_ERROR;
  rc = getaddrinfo(argv[1], argv[3], &addr_hints, &host);
  if (rc < 0)
    return TEST_ERROR;
  sock = socket(host->ai_family, host->ai_socktype, host->ai_protocol);
  if (sock < 0)
    return TEST_ERROR;

  errno = 0;
  rc = connect(sock, host->ai_addr, host->ai_addrlen);
  if (rc < 0 )
    return TEST_ERROR;

  errno = 0;
  rc = send(sock, MSG_STRING, MSG_LEN, sflgs);
  result = (rc < 0 ? TEST_FAIL : TEST_SUCCESS);

  fprintf(stderr, "%d %d %d\n", result, result ? errno : rc, getpid());
  return result;

}
