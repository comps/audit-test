#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <fcntl.h>
#include <pwd.h>
#include <grp.h>
#include <pthread.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/msg.h>
#include <sys/ipc.h>
#include <errno.h>

#define MSGKEY1 999
#define MSGKEY2 998

mode_t user_access[]  = {S_IRUSR, S_IWUSR, S_IRUSR | S_IWUSR, 0};
mode_t group_access[] = {S_IRGRP, S_IWGRP, S_IRGRP | S_IWGRP, 0};
mode_t other_access[] = {S_IROTH, S_IWOTH, S_IROTH | S_IWOTH, 0};

static char* socketname = "dactestsocket";
static char* output     = "./output";
static int   READ       = 0;
static int   WRITE      = 1;
static int   READWRITE  = 2;
static int   NOACCESS   = 3;

struct passwd *pw;
struct group  *gr;

struct msgbuf
{
  long mtype;
  char mtext[1];
} mbuf, tbuf;

uid_t uid;
gid_t gid;

int g_rc = 0;
int mmid;
int tmid;

/*
 * This is a cheap way to display the file
 * permissions and owner information.
 */
void socketinfo(void) {

  int fd;
  char readbuf[80];
  char lscommand[80];

  memset(readbuf, '\0', sizeof(readbuf));
  memset(lscommand, '\0', sizeof(lscommand));

  sprintf(lscommand, "ls -l %s >%s", socketname, output);
  system(lscommand);

  fd = open(output, O_RDONLY);
  read(fd, readbuf, sizeof(readbuf));
  printf(readbuf);
  printf("\n");
  close(fd);
}

/*
 * Perform read, write and execute checks.
 */
int access_check(mode_t access[], uid_t u, gid_t g) {

  int i = 0;
  int rc = 0;
  int sd = 0;
  char result[80];
  char *return_string;

  struct sockaddr_un sock;
  socklen_t len = 0;

  for (i = 0; i < 4; i++) {

    // request child process to create socket.
    tbuf.mtype=1;
    tbuf.mtext[0] = 'T';
    if ((rc = msgsnd(tmid, &tbuf, 1, MSG_NOERROR)) == -1) {
      printf("ERROR: msgsnd()\n");
      goto EXIT;
    }

    // Wait for socket
    mbuf.mtype=1;
    mbuf.mtext[0] = '\0';
    if ((rc = msgrcv(mmid, &mbuf, 1, 1, MSG_NOERROR)) == -1) {
      printf("ERROR: msgrcv()\n");
      goto EXIT;
    }

    // Get socket
    if ((sd = socket(PF_LOCAL, SOCK_STREAM, 0)) == -1) {
      printf("ERROR: socket()\n");
      rc = sd;
      goto EXIT;
    }

    // Set the socket file access mode.
    if ((rc = chmod(socketname, access[i])) == -1) {
      printf("ERROR: chmod(), %d\n", errno);
      goto EXIT;
    }

    // Set the socket file owner/group.
    if ((rc = chown(socketname, u, g)) == -1) {
      printf("ERROR: chown(), %d\n", errno);
      goto EXIT;
    }

    // Display current permission/owner info
    socketinfo();

    // Set effective user/group
    if ((rc = setegid(gid)) == -1) {
      printf("ERROR: setegid()\n");
      goto EXIT;
    }
    if ((rc = seteuid(uid)) == -1) {
      printf("ERROR: seteuid()\n");
      goto EXIT;
    }

    // Try connect as test user.
    sock.sun_family = AF_FILE;
    strcpy(sock.sun_path, socketname);
    len = sizeof(sock.sun_family) + strlen(sock.sun_path);

    return_string = strcpy(result, "Access:CONNECT");
    rc = connect(sd, (struct sockaddr *)&sock, len);
    if (rc == -1) {
      return_string = strcat(result, " | Allowed:NO ");
    } else {
      return_string = strcat(result, " | Allowed:YES");
    }
    if (((rc != -1) && (access[i] == access[READWRITE])) ||
	((rc == -1) && (access[i] == access[READ])) ||
	((rc != -1) && (access[i] == access[WRITE])) ||
	((rc == -1) && (access[i] == access[NOACCESS]))) {
      return_string = strcat(result, " | PASS\n");
    } else {
      return_string = strcat(result, " | FAIL\n");
      g_rc = -1;
    }

    printf(result);
    printf("\n");

    // Reset uid/gid back to root.
    if ((rc = seteuid(0)) == -1) {
      printf("ERROR: seteuid()\n");
      goto EXIT;
    }
    if ((rc = setegid(0)) == -1) {
      printf("ERROR: setegid()\n");
      goto EXIT;
    }

    close(sd);
    unlink(socketname);
  }

 EXIT:

  return (rc);
}

int main(int argc, char *argv[]) {

  int rc = 0;
  int sd = 0;

  uid_t uid_nobody;
  gid_t gid_nobody;

  pid_t pid = 0;

  struct sockaddr_un sock;
  socklen_t len = 0;

  if (argc != 2) {
    printf("Please provide username.\n");
    rc = -1;
    goto EXIT;
  }

  /*
   * Get test user uid/gid.
   */
  if ((pw = getpwnam(argv[1])) == NULL) {
    printf("Invalid username.\n");
    rc = -1;
    goto EXIT;
  }

  if ((gr = getgrgid(pw->pw_gid)) == NULL) {
    printf("Invalid group.\n");
    rc = -1;
    goto EXIT;
  }

  uid = pw->pw_uid;
  gid = gr->gr_gid;

  printf("\nSOCKET will be accessed as user: %s, group: %s\n\n",
	 pw->pw_name, gr->gr_name);

  /*
   * Get uid/gid for nobody
   */
  if ((pw = getpwnam("nobody")) == NULL) {
    printf("Invalid username.\n");
    rc = -1;
    goto EXIT;
  }
  uid_nobody = pw->pw_uid;
  if ((gr = getgrnam("nobody")) == NULL) {
    printf("Invalid group.\n");
    rc = -1;
    goto EXIT;
  }
  gid_nobody = gr->gr_gid;

  /*
   * Create msg queues
   */
  if ((mmid = msgget(MSGKEY1, 0666 | IPC_CREAT)) == -1) {
    printf("ERROR: msgget\n");
    rc = -1;
    goto EXIT;
  }
  if ((tmid = msgget(MSGKEY2, 0666 | IPC_CREAT)) == -1) {
    printf("ERROR: msgget\n");
    rc = -1;
    goto EXIT;
  }

  /*
   * Make sure socket file does not exist.
   */
  unlink(socketname);

  pid = fork();

  /*
   * Child process to create sockets
   */
  if (pid == 0) {

    while (msgrcv(tmid, &tbuf, 1, 1, MSG_NOERROR)) {

      if (tbuf.mtext[0] == 'T') {

	// Create Unix domain socket.
	sd = socket(PF_LOCAL, SOCK_STREAM, 0);

	sock.sun_family = AF_FILE;
	strcpy(sock.sun_path, socketname);
	len = sizeof(sock.sun_family) + strlen(sock.sun_path);
	rc = bind(sd, (struct sockaddr *)&sock, len);
	rc = listen(sd, 5);

	// Notify parent that socket file is created.
	mbuf.mtype=1;
	mbuf.mtext[0] = 'C';
	msgsnd(mmid, &mbuf, 1, MSG_NOERROR);
      } else {
	// Cleanup and leave.
	close(sd);
	unlink(socketname);
	exit(0);
      }
    }
  }

  /*
   * Test owner access modes
   */
  if ((rc = access_check(user_access, uid, gid_nobody)) == -1) {
    goto EXIT;
  }

  /*
   * Test group owner access modes
   */
  if ((rc = access_check(group_access, uid_nobody, gid)) == -1) {
    goto EXIT;
  }

  /*
   * Test other access modes
   */
  if ((rc = access_check(other_access, uid_nobody, gid_nobody)) == -1) {
    goto EXIT;
  }

  rc = 0;

 EXIT:

  /*
   * Tell child process to exit
   */
  tbuf.mtype=1;
  tbuf.mtext[0] = 'K';
  rc = msgsnd(tmid, &tbuf, 1, MSG_NOERROR);

  unlink(socketname);
  unlink(output);

  /*
  ** The reason for 2 return codes:
  ** g_rc represents a failure of the tested function.
  ** In this case, we want to continue testing the
  ** remaining functions.
  ** rc represents a failure of the test case itself.
  ** In other words, something in the setup is invalid.
  ** When that happens, the test stops executing and
  ** returns immediately.
  ** So rc may be 0, meaning the test ran completely.
  ** However, g_rc may be -1, indicating one or more
  ** of the tested functions failed.
  */
  if (g_rc != 0) {
    return (g_rc);
  } else {
    return (rc);
  }
}
