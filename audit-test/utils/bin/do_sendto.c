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

int main(int argc, char **argv)
{
  int rc, result;
  struct addrinfo *host = NULL;
  struct addrinfo addr_hints;
  int sock;

  if (argc != 3) {
    fprintf(stderr, "Usage:\n%s <host> <port>\n", argv[0]);
    return TEST_ERROR;
  }

  memset(&addr_hints, 0, sizeof(addr_hints));
  addr_hints.ai_socktype = SOCK_DGRAM;
  addr_hints.ai_protocol = IPPROTO_UDP;
  rc = getaddrinfo(argv[1], argv[2], &addr_hints, &host);
  if (rc < 0)
    return TEST_ERROR;
  sock = socket(host->ai_family, host->ai_socktype, host->ai_protocol);
  if (sock < 0)
    return TEST_ERROR;

  errno = 0;
  rc = sendto(sock, MSG_STRING, MSG_LEN, 0, host->ai_addr, host->ai_addrlen);
  result = (rc < 0 ? TEST_FAIL : TEST_SUCCESS);

  printf("%d %d %d\n", result, result ? errno : rc, getpid());
  return result;
}
