/* Copyright (c) 2015 Red Hat, Inc. All rights reserved.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of version 2 the GNU General Public License as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef NETHELPERS_H
#define NETHELPERS_H
/* bind the socket to the specified port - to be used by network clients
 * doing connect() / sendto(), binding their (source) port */
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

/* set the tcp_syn_retries (TCP_SYNCNT) for the (client) socket so that
 * connection attempts abort sooner/later */
#include <netinet/tcp.h>
int set_syn_retries(int s)
{
    int retries;
    char *envval = getenv("TCP_SYN_RETRIES");
    if (envval) {
        retries = atoi(envval);
    } else {
        fprintf(stderr, "warning: env var TCP_SYN_RETRIES not set\n");
        return -1;
    }
    if (retries < 1)
        return -1;
    if (setsockopt(s, IPPROTO_TCP, TCP_SYNCNT, &retries, sizeof(retries)) == -1)
        return -1;
    return retries;
}

/* schedule a SIGALRM-based syscall interrupt, for the listening (server)
 * socket to "time out" after a given time */
#include <signal.h>
static void dummy_handler(int signum) { return; }
int set_listen_timeout(void)
{
    int timeout;
    char *envval = getenv("LISTEN_TIMEOUT");
    if (envval) {
        timeout = atoi(envval);
    } else {
        fprintf(stderr, "warning: env var LISTEN_TIMEOUT not set\n");
        return -1;
    }
    if (timeout < 1)
        return -1;
    if (siginterrupt(SIGALRM, 1) == -1)
        return -1;
    struct sigaction act = { .sa_handler = dummy_handler };
    if (sigaction(SIGALRM, &act, NULL) == -1)
        return -1;
    return alarm(timeout);
}
#endif

/* vim: set sts=4 sw=4 et : */
