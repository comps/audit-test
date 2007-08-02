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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <netdb.h>
#include <unistd.h>
#include <getopt.h>
#include <time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <selinux/selinux.h>
#include <selinux/context.h>

/* XXX - Notes
 *
 * to compile this file
 *  # gcc -Wall -g -o server -lselinux server.c
 *
 * to run the server
 *  # ./server <port>
 *
 * send a command (using telnet)
 *  # telnet ::1 <port>
 *  Trying ::1...
 *  Connected to localhost.
 *  Escape character is '^]'.
 *  exit;
 *  Connection closed by foreign host.
 *
 * send a command (using netcat, scripted)
 *  # echo "exit;" | nc -q 1 localhost <port>
 *
 */

/* XXX - ToDo List
 *
 * 1. better documentation/comments
 * 2. general cleanup/fixup
 *
 */

/* constants */
#define NET_TIMEOUT_DEFAULT             0       /* seconds */

/* lock file */
#define LCK_FILE                        "/var/lock/lblnet_tst_server"

/* control socket constants */
#define CTL_SOCK_PORT_DEFAULT           5000
#define CTL_SOCK_LISTEN_QUEUE           1
#define CTL_SOCK_BUF_SIZE               4096    /* bytes */

/* control message constats */
#define CTL_MSG_BAD_CHARS               "\n\r"

/* status message macro */
#define SMSG_NONE                       0
#define SMSG_ERR                        1
#define SMSG_WARN                       2
#define SMSG_NOTICE                     3
#define SMSG_ALL                        16
#define SMSG(_level,_cmd) \
  do {						\
    if (_level <= smsg_level)			\
      _cmd;					\
  } while (0)


/* network operation timeout value */
unsigned int net_timeout_sec = NET_TIMEOUT_DEFAULT;

/* visible status message watermark */
unsigned int smsg_level = SMSG_ERR;

/**
 * hlp_usage - Print a usage message and exit
 * @name - program name
 *
 * Description:
 * Print a usage message and exit with a value of 1.
 *
 */
void hlp_usage(char *name)
{
  SMSG(SMSG_ERR,
       fprintf(stderr,
	       "usage: %s [-i] [-p <port>] [-q] [-t <secs>] [-v]\n",
	       (name != NULL ? name : "?")));
  exit(1);
}

/**
 * net_hlp_timeout_rcv - Wait for data on a socket
 * @sock: socket
 *
 * Description:
 * Uses select() to wait for data on a socket.  Returns the return value from
 * select() or 1 if the value in net_timeout_sec is 0 (no timeout);
 *
 */
int net_hlp_timeout_rcv(int sock)
{
  struct timeval timeout;
  fd_set sock_fdset;

  if (net_timeout_sec == 0)
    return 1;

  timeout.tv_sec = net_timeout_sec;
  timeout.tv_usec = 0;
	
  FD_ZERO(&sock_fdset);
  FD_SET(sock, &sock_fdset);

  return select(sock + 1, &sock_fdset, NULL, NULL, &timeout);
}

/* net_hlp_socket_close - Shutdown and close a socket
 * @sock: pointer to socket
 *
 * Description:
 * Shutdown and close @sock.
 *
 */
void net_hlp_socket_close(int *sock)
{
  if (sock < 0)
    return;

  shutdown(*sock, SHUT_RDWR);
  close(*sock);
  *sock = -1;
}

/**
 * ctl_hlp_sendrc - Write a return value to a socket
 * @sock: socket
 * @rc: return value
 *
 * Description:
 * Write the value in @rc as an ASCII string to @sock.
 *
 */
void ctl_hlp_sendrc(int sock, int rc)
{
  FILE *fp = fdopen(sock, "a");
  fprintf(fp, "%d", rc);
  fflush(fp);
}

/**
 * ctl_hlp_sendstr - Write a string to a socket
 * @sock: socket
 * @str: string to write to the socket
 *
 * Description:
 * Write the value in @str as an ASCII string to @sock.
 *
 */
void ctl_hlp_sendstr(int sock, const char *str)
{
  FILE *fp = fdopen(sock, "a");
  fprintf(fp, "%s", str);
  fflush(fp);
}

/**
 * ctl_echo - Handle the "echo" control message
 * @sock: socket
 * @param: parameter string
 *
 * Description:
 * Echo the parameter string back to the remote host.  The control message
 * format:
 *
 *  echo:<message>
 *
 * This is intended as a debugging control message only.
 *
 */
void ctl_echo(int sock, char *param)
{
  int rc = write(sock, param, strlen(param) + 1);
  if (rc < 0)
    SMSG(SMSG_WARN,
	 fprintf(stderr, "warning(echo): failed to write to the socket (%d)\n",
		 errno));
}

/**
 * ctl_sleep - Handle the "sleep" control message
 * @sock: socket
 * @param: parameter string
 *
 * Description:
 * Sleep for the specified number of seconds.  The control message format:
 *
 *  sleep:<seconds>
 *
 */
void ctl_sleep(int sock, char *param)
{
  int rc = sleep(atoi(param));
  if (rc > 0)
    SMSG(SMSG_WARN,
	 fprintf(stderr,
		 "warning(sleep): failed to sleep for the full time\n"));
}

/**
 * ctl_lock - Handle the "lock" control message
 * @sock: socket
 * @param: parameter string
 *
 * Description:
 * Set, release, or query the lock, return the number of seconds until the lock
 * expires (a value of "0" means you have the lock) or nothing on error.  The
 * control message format:
 *
 *  lock:set,<timeout>
 *  lock:release
 *  lock:query
 *
 */
void ctl_lock(int sock, char *param)
{
  int rc;
  char *cmd_str, *timeout_str;
  unsigned int timeout = 0;
  time_t time_now, time_lock = 0;
  FILE *lock_file;

  if (param == NULL) {
    SMSG(SMSG_ERR, fprintf(stderr, "error(lock): bad message\n"));
    return;
  }

  /* XXX - at some point we should probably do flock locking, or something
   *       similar, on the lock file but i'm lazy and this should work fairly
   *       well for right now ... */

  /* parse the control message */
  cmd_str = strtok(param, ",");
  if (strcasecmp(cmd_str, "set") == 0) {
    timeout_str = strtok(NULL, ",");
    timeout = atoi(timeout_str);
  }

  /* perform the command */
  time_now = time(NULL);
  if (strcasecmp(cmd_str, "set") == 0) {
    /* try to read the lock */
    lock_file = fopen(LCK_FILE, "r+");
    if (lock_file == NULL) {
      lock_file = fopen(LCK_FILE, "w");
      if (lock_file == NULL) {
	SMSG(SMSG_ERR,
	     fprintf(stderr, "error(lock): unable to access the lock file\n"));
	return;
      }
      time_lock = time_now;
    } else {
      rc = fscanf(lock_file, "%ld", &time_lock);
      if (rc != 1 && rc != EOF) {
	SMSG(SMSG_ERR,
	     fprintf(stderr, "error(lock): unable to read the lock file\n"));
	return;
      }
    }
    if (time_lock <= time_now) {
      /* we have the lock */
      rewind(lock_file);
      fprintf(lock_file, "%ld", time_now + timeout);
      ctl_hlp_sendrc(sock, 0);
    } else {
      /* someone else has the lock */
      ctl_hlp_sendrc(sock, time_lock - time_now);
    }
    fclose(lock_file);
  } else if  (strcasecmp(cmd_str, "release") == 0) {
    /* release the "lock" */
    lock_file = fopen(LCK_FILE, "w");
    if (lock_file == NULL) {
      SMSG(SMSG_ERR,
	   fprintf(stderr, "error(lock): unable to access the lock file\n"));
      return;
    }
    fprintf(lock_file, "%ld", time_now);
    fclose(lock_file);
  } else if  (strcasecmp(cmd_str, "query") == 0) {
    /* check the "lock" */
    lock_file = fopen(LCK_FILE, "r");
    if (lock_file == NULL) {
      SMSG(SMSG_ERR,
	   fprintf(stderr, "error(lock): unable to access the lock file\n"));
      return;
    }
    rc = fscanf(lock_file, "%ld", &time_lock);
    if (rc != 1 && rc != EOF) {
      SMSG(SMSG_ERR,
	   fprintf(stderr, "error(lock): unable to read the lock file\n"));
      return;
    }
    if (time_lock <= time_now)
      ctl_hlp_sendrc(sock, 0);
    else
      ctl_hlp_sendrc(sock, time_lock - time_now);
    fclose(lock_file);
  } else
    return;
}

/**
 * ctl_sendrand - Handle the "sendrand" control message
 * @sock: socket
 * @param: parameter string
 *
 * Description:
 * Send a message to a remote host and return an error code on @sock as a
 * positive ASCII integer.  The control message format:
 *
 *  sendrand:<host>,tcp|udp,<port>,<bytes>
 *
 */
void ctl_sendrand(int sock, struct sockaddr_storage *peer_addr, char *param)
{
  int rc;
  char *host_str, *proto_str, *port_str, *bytes_str;
  struct addrinfo *host = NULL;
  struct addrinfo addr_hints;
  unsigned int bytes;
  int data_sock = -1;

  unsigned int iter;
  unsigned char byte;

  if (param == NULL) {
    SMSG(SMSG_ERR, fprintf(stderr, "error(sendrand): bad message\n"));
    rc = EINVAL;
    goto sendrand_return;
  }

  /* parse the control message */
  host_str = strtok(param, ",");
  proto_str = strtok(NULL, ",");
  port_str = strtok(NULL, ",");
  bytes_str = strtok(NULL, "");
  if (bytes_str == NULL) {
    SMSG(SMSG_ERR, fprintf(stderr, "error(sendrand): bad message\n"));
    rc = EINVAL;
    goto sendrand_return;
  }
  memset(&addr_hints, 0, sizeof(addr_hints));
  if (strcasecmp(proto_str, "tcp") == 0) {
    addr_hints.ai_socktype = SOCK_STREAM;
    addr_hints.ai_protocol = IPPROTO_TCP;
  } else if (strcasecmp(proto_str, "udp") == 0) {
    addr_hints.ai_socktype = SOCK_DGRAM;
    addr_hints.ai_protocol = IPPROTO_UDP;
  } else {
    SMSG(SMSG_ERR, fprintf(stderr, "error(sendrand): bad message\n"));
    rc = EINVAL;
    goto sendrand_return;
  }
  rc = getaddrinfo(host_str, port_str, &addr_hints, &host);
  if (rc < 0) {
    SMSG(SMSG_ERR,
	 fprintf(stderr, "error(sendrand): name resolution failure (%s)\n",
		 host_str));
    rc = EFAULT;
    goto sendrand_return;
  }
  bytes = atoi(bytes_str);

  /* if the peer is using ipv6 link-local addressing then copy the scope-id
   * from the peer's control socket to the new data socket */
  if (host->ai_family == AF_INET6 && 
      IN6_IS_ADDR_LINKLOCAL(
		         &((struct sockaddr_in6 *)host->ai_addr)->sin6_addr)) {
    ((struct sockaddr_in6 *)host->ai_addr)->sin6_scope_id =	\
      ((struct sockaddr_in6 *)peer_addr)->sin6_scope_id;
    SMSG(SMSG_NOTICE,
	 fprintf(stderr, "notice(sendrand): adjusted scope-id\n"));
  }

  /* connect to the remote host */
  data_sock = socket(host->ai_family, host->ai_socktype, host->ai_protocol);
  if (data_sock < 0) {
    SMSG(SMSG_ERR,
	 fprintf(stderr, "error(sendrand): failed to create socket (%d)\n",
		 errno));
    rc = errno;
    goto sendrand_return;
  }
  rc = connect(data_sock, host->ai_addr, host->ai_addrlen);
  if (rc < 0) {
    SMSG(SMSG_ERR,
	 fprintf(stderr,
		 "error(sendrand): unable to connect to remote host (%d)\n",
		 errno));
    rc = errno;
    goto sendrand_return;
  }

  /* send the data */
  for (iter = 0, byte = 'a'; iter < bytes; iter++) {
    rc = write(data_sock, &byte, 1);
    if (rc != 1)
      SMSG(SMSG_WARN,
	   fprintf(stderr, "warning(sendrand): write to socket failed (%d)\n",
		   errno));
    if (byte++ == 'z')
      byte = 'a';
  }

  rc = 0;

  /* cleanup */
 sendrand_return:
  ctl_hlp_sendrc(sock, rc);
  if (host != NULL)
    freeaddrinfo(host);
  net_hlp_socket_close(&data_sock);
}

/**
 * ctl_recv - Handle the "recv" control message
 * @sock: socket
 * @param: parameter string
 *
 * Description:
 * Open a port and wait for input, return an error code on @sock as a positive
 * ASCII integer.  The control message format:
 *
 *  recv:ipv4|ipv6,tcp|udp,<port>,<bytes>
 *
 */
void ctl_recv(int sock, char *param)
{
  int rc;
  char *inet_family_str, *proto_str, *port_str, *bytes_str;
  int inet_family, data_sock_type, data_sock_proto, bytes;
  unsigned short port;
  int data_sock = -1;
  int child_sock;
  struct sockaddr_storage data_sockaddr;
  struct sockaddr_in *data_sockaddr4 = (struct sockaddr_in *)&data_sockaddr;
  struct sockaddr_in6 *data_sockaddr6 = (struct sockaddr_in6 *)&data_sockaddr;
  char *recv_buf = NULL;
  size_t recv_buf_len = 0;
  size_t bytes_recv = 0;
  int bool_true = 1;

  if (param == NULL) {
    SMSG(SMSG_ERR, fprintf(stderr, "error(recv): bad message\n"));
    rc = EINVAL;
    goto recv_return;
  }

  /* parse the control message */
  inet_family_str = strtok(param, ",");
  proto_str = strtok(NULL, ",");
  port_str = strtok(NULL, ",");
  bytes_str = strtok(NULL, "");
  if (bytes_str == NULL) {
    SMSG(SMSG_ERR, fprintf(stderr, "error(recv): bad message\n"));
    rc = EINVAL;
    goto recv_return;
  }
  if (strcasecmp(inet_family_str, "ipv4") == 0) {
    inet_family = AF_INET;
  } else if (strcasecmp(inet_family_str, "ipv6") == 0) {
    inet_family = AF_INET6;
  } else {
    SMSG(SMSG_ERR, fprintf(stderr, "error(recv): bad message\n"));
    rc = EINVAL;
    goto recv_return;
  }
  if (strcasecmp(proto_str, "tcp") == 0) {
    data_sock_type = SOCK_STREAM;
    data_sock_proto = IPPROTO_TCP;
  } else if (strcasecmp(proto_str, "udp") == 0) {
    data_sock_type = SOCK_DGRAM;
    data_sock_proto = IPPROTO_UDP;
  } else {
    SMSG(SMSG_ERR, fprintf(stderr, "error(recv): bad message\n"));
    rc = EINVAL;
    goto recv_return;
  }
  port = atoi(port_str);
  bytes = atoi(bytes_str);

  /* create and bind the socket */
  data_sock = socket(inet_family, data_sock_type, data_sock_proto);
  if (data_sock < 0) {
    SMSG(SMSG_ERR,
	 fprintf(stderr, "error(recv): failed to create socket (%d)\n",
		 errno));
    rc = errno;
    goto recv_return;
  }
  rc = setsockopt(data_sock, SOL_SOCKET, SO_REUSEADDR, &bool_true, sizeof(int));
  if (rc < 0) {
    SMSG(SMSG_ERR,
	 fprintf(stderr, "error(recv): failed to configure socket (%d)\n",
		 errno));
    goto recv_return;
  }
  memset(&data_sockaddr, 0, sizeof(data_sockaddr));
  data_sockaddr.ss_family = inet_family;
  switch (inet_family) {
  case AF_INET:
    data_sockaddr4->sin_port = htons(port);
    break;
  case AF_INET6:
    data_sockaddr6->sin6_port = htons(port);
    break;
  }
  rc = bind(data_sock,
	    (struct sockaddr *)&data_sockaddr,
	    sizeof(data_sockaddr));
  if (rc < 0) {
    SMSG(SMSG_ERR,
	 fprintf(stderr, "error(recv): failed to bind the socket (%d)\n",
		 errno));
    rc = errno;
    goto recv_return;
  }
  if (data_sock_type == SOCK_STREAM) {
    /* configure the socket for one incoming connection at a time and wait for
     * a new connection to arrive */
    rc = listen(data_sock, 1);
    if (rc < 0) {
      SMSG(SMSG_ERR,
	   fprintf(stderr,
		   "error(recv): failed to listen on the socket (%d)\n",
		   errno));
      rc = errno;
      goto recv_return;
    }
    rc = net_hlp_timeout_rcv(data_sock);
    if (rc < 0) {
      SMSG(SMSG_ERR,
	   fprintf(stderr, "error(recv): select failed while waiting (%d)\n",
		   errno));
      rc = errno;
      goto recv_return;
    } else if (rc == 0) {
      SMSG(SMSG_NOTICE,
	   fprintf(stderr,
		   "notice(recv): timeout while waiting for a connection\n"));
      rc = EAGAIN;
      goto recv_return;
    }
    child_sock = accept(data_sock, NULL, 0);
    if (child_sock < 0) {
      SMSG(SMSG_WARN,
	   fprintf(stderr,
		   "error(recv): failed to accept a connection (%d)\n",
		   errno));
      rc = errno;
      goto recv_return;
    }

    /* swap sockets and close the parent */
    net_hlp_socket_close(&data_sock);
    data_sock = child_sock;
  }

  /* get the data from the network */
  rc = 1;
  while ((rc > 0) && (bytes_recv < bytes)) {
    do {
      recv_buf_len += bytes;
      recv_buf = realloc(recv_buf, bytes + 1);
      if (recv_buf == NULL) {
	SMSG(SMSG_ERR,
	     fprintf(stderr, "error(recv): out of memory (%zu)\n",
		     recv_buf_len));
	rc = ENOMEM;
	goto recv_return;
      }
      rc = net_hlp_timeout_rcv(data_sock);
      if (rc < 0) {
	SMSG(SMSG_ERR,
	     fprintf(stderr, "error(recv): select failed while waiting (%d)\n",
		     errno));
	rc = errno;
	goto recv_return;
      } else if (rc == 0) {
	SMSG(SMSG_NOTICE,
	     fprintf(stderr,
		     "notice(recv): timeout while waiting for data\n"));
	rc = EAGAIN;
	goto recv_return;
      }
      rc = recv(data_sock, recv_buf, recv_buf_len, MSG_PEEK);
    } while (rc == recv_buf_len);
    rc = recv(data_sock, recv_buf, recv_buf_len, 0);
    if (rc > 0)
      bytes_recv += rc;
    recv_buf_len = 0;
  }

  rc = 0;

 recv_return:
  ctl_hlp_sendrc(sock, rc);
  net_hlp_socket_close(&data_sock);
}

/**
 * ctl_sockcon - Set the SELinux context for new sockets
 * @sock: socket
 * @param: parameter string
 *
 * Description:
 * Set the SELinux context for newly created sockets and return an error code
 * on @sock as a positive ASCII integer.  The control message format:
 *
 *  sockcon:full|mls,<context>
 *
 */
void ctl_sockcon(int sock, char *param)
{
  int rc;
  char *type_str, *ctx_str;
  security_context_t sctx = NULL, sctx_tmp;
  context_t ctx = NULL;

  /* XXX - check to make sure we are not leaking security_context_t vars */

  if (param == NULL) {
    SMSG(SMSG_ERR, fprintf(stderr, "error(sockcon): bad message\n"));
    rc = EINVAL;
    goto sockcon_return;
  }

  /* parse the control message */
  type_str = strtok(param, ",");
  ctx_str = strtok(NULL, "");
  if (ctx_str == NULL) {
    SMSG(SMSG_ERR, fprintf(stderr, "error(sockcon): bad message\n"));
    rc = EINVAL;
    goto sockcon_return;
  }

  /* create the new socket context */
  if (strcasecmp(type_str, "full") == 0) {
    sctx = strdup(ctx_str);
  } else if (strcasecmp(type_str, "mls") == 0) {
    /* XXX - need to find better "rc" values for the SELinux functions on
     * failure */
    rc = getcon(&sctx);
    if (rc < 0) {
      SMSG(SMSG_ERR,
	   fprintf(stderr, "error(sockcon): failed to get current context\n"));
      goto sockcon_return;
    }
    ctx = context_new(sctx);
    if (ctx == NULL) {
      SMSG(SMSG_ERR,
	   fprintf(stderr, "error(sockcon): failed to convert the context\n"));
      rc = -1;
      goto sockcon_return;
    }
    rc = context_range_set(ctx, ctx_str);
    if (rc < 0) {
      SMSG(SMSG_ERR,
	   fprintf(stderr, "error(sockcon): failed to modify context\n"));
      goto sockcon_return;
    }
    sctx_tmp = context_str(ctx);
    if (sctx_tmp == NULL) {
      SMSG(SMSG_ERR,
	   fprintf(stderr, "error(sockcon): failed to convert the context\n"));
      rc = -1;
      goto sockcon_return;
    }
    sctx = strdup(sctx_tmp);
  } else {
    SMSG(SMSG_ERR, fprintf(stderr, "error(sockcon): bad message\n"));
    rc = EINVAL;
    goto sockcon_return;
  }
  if (sctx == NULL) {
    SMSG(SMSG_ERR,
	 fprintf(stderr, "error(sockcon): out of memory\n"));
    rc = ENOMEM;
    goto sockcon_return;
  }

  /* set socket context */
  rc = setsockcreatecon(sctx);
  if (rc < 0) {
    SMSG(SMSG_ERR,
	 fprintf(stderr,
		 "error(sockcon): failed to set the socket context (%s)\n",
		 sctx));
    goto sockcon_return;
  }

  rc = 0;

 sockcon_return:
  ctl_hlp_sendrc(sock, rc);
  if (ctx != NULL)
    context_free(ctx);
}

/**
 * ctl_getcon - Get the SELinux context of the running process
 * @sock: socket
 * @param: parameter string
 *
 * Description:
 * Return the SELinux context of the current instance.  The control message
 * format:
 *
 *  getcon:full|mls
 *
 */
void ctl_getcon(int sock, char *param)
{
  int rc;
  char *type_str;
  const char *ctx_str = NULL;
  security_context_t sctx = NULL;
  context_t ctx = NULL;

  /* XXX - check to make sure we are not leaking security_context_t vars */

  if (param == NULL) {
    SMSG(SMSG_ERR, fprintf(stderr, "error(getcon): bad message\n"));
    goto getcon_return;
  }

  /* parse the control message */
  type_str = strtok(param, ",");
  if (type_str == NULL) {
    SMSG(SMSG_ERR, fprintf(stderr, "error(sockcon): bad message\n"));
    rc = EINVAL;
    goto getcon_return;
  }

  /* fetch the current application context/domain */
  rc = getcon(&sctx);
  if (rc < 0) {
    SMSG(SMSG_ERR,
	 fprintf(stderr, "error(getcon): unable to get the context\n"));
    goto getcon_return;
  }
  ctx = context_new(sctx);
  if (ctx == NULL) {
    SMSG(SMSG_ERR,
	 fprintf(stderr, "error(getcon): unable to get the context\n"));
    goto getcon_return;
  }

  /* get the desired context portion */
  if (strcasecmp(type_str, "full") == 0)
    ctx_str = context_str(ctx);
  else if (strcasecmp(type_str, "mls") == 0)
    ctx_str = context_range_get(ctx);
  else {
    SMSG(SMSG_ERR, fprintf(stderr, "error(getcon): bad message\n"));
    goto getcon_return;
  }
  if (ctx_str == NULL) {
    SMSG(SMSG_ERR,
	 fprintf(stderr, "error(getcon): unable to parse the context\n"));
    goto getcon_return;
  }

  /* send the context back to the client */
  ctl_hlp_sendstr(sock, ctx_str);

 getcon_return:
  if (ctx)
    context_free(ctx);
}

/*
 * main
 */
int main(int argc, char *argv[])
{
  int rc;
  int arg_iter;
  int run_loop = 1;
  unsigned short ctl_port = CTL_SOCK_PORT_DEFAULT;
  int ctl_sock;
  int rem_sock = -1;
  struct sockaddr_in6 ctl_sockaddr;
  struct sockaddr_storage peer_addr;
  socklen_t peer_addr_len;
  char *msg_buf = NULL;
  char *msg_buf_next;
  char *recv_buf = NULL;
  size_t recv_buf_len = 0;
  char *ctl_cmd, *ctl_param;

  int bool_true = 1;
  size_t tmp_sze;
  int inetd_flag = 0;

  /* command line arguments */
  do {
    arg_iter = getopt(argc, argv, "ip:qt:v");
    switch (arg_iter) {
    case 'i':
      /* [x]inetd flag */
      inetd_flag = 1;
      break;
    case 'p':
      /* control message port */
      ctl_port = atoi(optarg);
      break;
    case 'q':
      /* quiet */
      smsg_level = SMSG_NONE;
      break;
    case 't':
      /* network timeout */
      net_timeout_sec = atoi(optarg);
      break;
    case 'v':
      /* verbose */
      if (smsg_level < SMSG_ALL)
	smsg_level++;
      break;
    case '?':
      hlp_usage(argv[0]);
      break;
    }
  } while (arg_iter > 0);

  if (!inetd_flag) {
    /* create, bind, and start listening on the control socket */
    ctl_sock = socket(AF_INET6, SOCK_STREAM, IPPROTO_TCP);
    if (ctl_sock < 0) {
      SMSG(SMSG_ERR,
	   fprintf(stderr, "error: failed to create control socket (%d)\n",
		   errno));
      return 1;
    }
    rc = setsockopt(ctl_sock,
		    SOL_SOCKET, SO_REUSEADDR, &bool_true, sizeof(int));
    if (rc < 0) {
      SMSG(SMSG_ERR,
	   fprintf(stderr, "error: failed to configure control socket (%d)\n",
		   errno));
      return 1;
    }
    memset(&ctl_sockaddr, 0, sizeof(ctl_sockaddr));
    ctl_sockaddr.sin6_family = AF_INET6;
    ctl_sockaddr.sin6_port = htons(ctl_port);
    rc = bind(ctl_sock,
	      (struct sockaddr *)&ctl_sockaddr, sizeof(ctl_sockaddr));
    if (rc < 0) {
      SMSG(SMSG_ERR,
	   fprintf(stderr, "error: failed to bind the control socket (%d)\n",
		   errno));
      return 1;
    }
    rc = listen(ctl_sock, CTL_SOCK_LISTEN_QUEUE);
    if (rc < 0) {
      SMSG(SMSG_ERR,
	   fprintf(stderr,
		   "error: failed to listen on the control socket (%d)\n",
		   errno));
      return 1;
    }
  } else {
    /* dup stdin to rem_sock */
    rem_sock = dup(fileno(stdin));
    if (rem_sock < 0) {
      SMSG(SMSG_ERR,
	   fprintf(stderr, "error: failed to duplicate control fd (%d)\n",
		   errno));
      return 1;
    }
  }

  /* loop on incoming messages */
  while (run_loop) {
    if (rem_sock < 0 && !inetd_flag) {
      /* get a new connection and don't honor the timeout here, if we are not
       * running in [x]inetd mode assume we are running as a daemon */
      peer_addr_len = sizeof(peer_addr);
      rem_sock = accept(ctl_sock,
			(struct sockaddr *)&peer_addr,
			&peer_addr_len);
      if (rem_sock < 0) {
	SMSG(SMSG_WARN,
	     fprintf(stderr,
		     "warning: failed to accept new control connection (%d)\n",
		     errno));
	continue;
      }
    }

    /* get a new message */
    if ((msg_buf == NULL) || (strchr(msg_buf, ';') == NULL)) {
      /* get more data from the network */
      do {
	recv_buf_len += CTL_SOCK_BUF_SIZE - 1;
	recv_buf = realloc(recv_buf, recv_buf_len + 1);
	if (recv_buf == NULL) {
	  SMSG(SMSG_ERR,
	       fprintf(stderr, "error: out of memory (%zu)\n",
		       recv_buf_len));
	  return 1;
	}
	rc = net_hlp_timeout_rcv(rem_sock);
	if (rc < 0) {
	  SMSG(SMSG_ERR,
	       fprintf(stderr, "error: select failed while waiting (%d)\n",
		       errno));
	  return 1;
	} else if (rc == 0) {
	  SMSG(SMSG_NOTICE,
	       fprintf(stderr, "notice: timeout while waiting for data\n"));
	  return 0;
	}
	rc = recv(rem_sock, recv_buf, recv_buf_len, MSG_PEEK);
      } while (rc == recv_buf_len);
      memset(recv_buf, 0, recv_buf_len + 1);
      rc = recv(rem_sock, recv_buf, recv_buf_len, 0);
      if (rc == 0) {
	/* connection is closed */
	if (!inetd_flag)
	  /* reset for a new connection */
	  net_hlp_socket_close(&rem_sock);
	else
	  /* we die and let [x]inetd handle any new connections */
	  return 0;
	continue;
      } else if (rc < 0) {
	SMSG(SMSG_WARN,
	     fprintf(stderr,
		     "warning: failed to read the control message (%d)\n",
		     errno));
	recv_buf_len = 0;
	continue;
      }

      /* clean up the data */
      do {
	tmp_sze = strcspn(recv_buf, CTL_MSG_BAD_CHARS);
	if (tmp_sze < strlen(recv_buf)) {
	  char *bad_str = recv_buf + tmp_sze;
	  char *good_str = bad_str + strspn(bad_str, CTL_MSG_BAD_CHARS);
	  memmove(bad_str, good_str, strlen(good_str) + 1);
	}
      } while (tmp_sze < strlen(recv_buf));

      /* add the data to the message buffer */
      if (msg_buf != NULL) {
	msg_buf = realloc(msg_buf, strlen(msg_buf) + strlen(recv_buf) + 1);
	if (msg_buf == NULL) {
	  SMSG(SMSG_ERR,
	       fprintf(stderr, "error: out of memory\n"));
	  return 1;
	}
	strcat(msg_buf, recv_buf);
      } else {
	msg_buf = recv_buf;
	recv_buf = NULL;
      }
      recv_buf_len = 0;
    }

    /* parse/handle the message buffer */
    while ((msg_buf_next = strchr(msg_buf, ';')) != NULL) {
      *msg_buf_next++ = '\0';

      ctl_cmd = strtok(msg_buf, ":");
      ctl_param = strtok(NULL, "");
      if (ctl_cmd != NULL) {
	if (strcasecmp(ctl_cmd, "exit") == 0) {
	  run_loop = 0;
	} else if (strcasecmp(ctl_cmd, "echo") == 0) {
	  ctl_echo(rem_sock, ctl_param);
	} else if (strcasecmp(ctl_cmd, "sleep") == 0) {
	  ctl_sleep(rem_sock, ctl_param);
	} else if (strcasecmp(ctl_cmd, "lock") == 0) {
	  ctl_lock(rem_sock, ctl_param);
	} else if (strcasecmp(ctl_cmd, "sendrand") == 0) {
	  ctl_sendrand(rem_sock, &peer_addr, ctl_param);
	} else if (strcasecmp(ctl_cmd, "recv") == 0) {
	  ctl_recv(rem_sock, ctl_param);
	} else if (strcasecmp(ctl_cmd, "sockcon") == 0) {
	  ctl_sockcon(rem_sock, ctl_param);
	} else if (strcasecmp(ctl_cmd, "getcon") == 0) {
	  ctl_getcon(rem_sock, ctl_param);
	} else {
	  SMSG(SMSG_WARN,
	       fprintf(stderr, "warning: unknown control message (%s)\n",
		       ctl_cmd));
	}
      }
      
      memmove(msg_buf, msg_buf_next, strlen(msg_buf_next) + 1);
    }
  }

  /* cleanup */
  net_hlp_socket_close(&ctl_sock);
  if (recv_buf)
    free(recv_buf);

  return 0;
}
