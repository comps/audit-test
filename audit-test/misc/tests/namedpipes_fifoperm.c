#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <stdio.h>
#include <pwd.h>
#include <grp.h>
#include <pthread.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>

mode_t user_access[]  = {S_IRUSR, S_IWUSR};
mode_t group_access[] = {S_IRGRP, S_IWGRP};
mode_t other_access[] = {S_IROTH, S_IWOTH};

static char* fifoname = "./dactestfifo";
static char* output   = "./output";
static int   READ    = 0;
static int   WRITE   = 1;

struct passwd *pw;
struct group *gr;

uid_t uid;
gid_t gid;

int g_rc  = 0;

pthread_mutex_t pt_mutex;
pthread_cond_t pt_cond;

void open_fifo(void *name) {
  int fd;
  int rc;
  char readbuff[10];

  fd = open((char *)name, O_RDWR);
  if (fd > 0) {
      if ((rc = pthread_cond_signal(&pt_cond)) != 0)
          pthread_exit(NULL);
      read(fd, readbuff, sizeof(readbuff));
      close(fd);
  } else
      pthread_exit(NULL);
  pthread_exit(NULL);
}

/*
 * This is a cheap way to display the file
 * permissions and owner information.
 */
void fifoinfo() {

    int fd;
    char readbuf[80];
    char lscommand[80];

    memset(readbuf, '\0', sizeof(readbuf));
    memset(lscommand, '\0', sizeof(lscommand));

    sprintf(lscommand, "ls -l %s >%s", fifoname, output);
    system(lscommand);

    fd = open(output, O_RDONLY);
    if (fd >= 0) {
      read(fd, readbuf, sizeof(readbuf));
      printf(readbuf);
      printf("\n");
      close(fd);
    }
}

/*
 * Perform read, write and execute checks.
 */
int access_check(mode_t access[]) {

    int i;
    int rc = 0;
    int fd;
    char result[80];
    pthread_t open_thread;
    struct timespec timeout;

    // Spawn a new thread to listen on the other end of the fifo and wait
    // for 5 seconds for the thread to open the fifo before running the tests
    pthread_mutex_lock(&pt_mutex);
    if (clock_gettime(CLOCK_REALTIME, &timeout) < 0) {
        rc = -1;
	goto EXIT;
    }
    timeout.tv_sec += 5;
    rc = pthread_create(&open_thread, NULL, (void*)&open_fifo, (void*)fifoname);
    if (rc != 0) {
	pthread_mutex_unlock(&pt_mutex);
        goto EXIT;
    }
    rc = pthread_cond_timedwait(&pt_cond, &pt_mutex, &timeout);
    pthread_mutex_unlock(&pt_mutex);
    if (rc != 0) {
        goto EXIT;
    }

    for (i = 0; i < 2; i++) {
        if ((rc = chmod(fifoname, access[i])) < 0) {
            goto EXIT;
        }

        if ((rc = setegid(gid)) < 0) {
            goto EXIT;
        }
        if ((rc = seteuid(uid)) < 0) {
            goto EXIT;
        }

        // Display current permission/owner info
        fifoinfo();

        // Try open for read
        fd = open(fifoname, O_RDONLY);

        if ((fd >= 0 && access[i] == access[READ]) ||
            (fd < 0 && access[i] != access[READ])) {
        } else {
	    g_rc = -1;
        }

        printf(result);

        if (fd >= 0) {
            if ((rc = close(fd)) < 0) {
                goto EXIT;
            }
        }

        // Try open for write
        fd = open(fifoname, O_WRONLY);

        if ((fd >= 0 && access[i] == access[WRITE]) ||
            (fd < 0 && access[i] != access[WRITE])) {
        } else {
	    g_rc = -1;
        }

        printf(result);

        if (fd >= 0) {
            if ((rc = close(fd)) < 0) {
                goto EXIT;
            }
        }

        printf("\n");

        // Reset uid/gid
        if ((rc = seteuid(0)) < 0) {
            goto EXIT;
        }
        if ((rc = setegid(0)) < 0) {
            goto EXIT;
        }
    }

    // Open FIFO and send exit message.
    fd = open(fifoname, O_RDWR);
    write(fd, "Exit\n\n", 6);
    close(fd);

EXIT:

    return (rc);

}

int main(int argc, char *argv[]) {

    int rc = 0;

    uid_t uid_nobody;
    gid_t gid_nobody;

    if (argc != 2) {
        printf("Please provide username.\n");
        rc = -1;
        goto EXIT;
    }

    /*
     * pthread setup
     */
    if ((rc = pthread_mutex_init(&pt_mutex, NULL)) != 0) {
      printf("Failed to initialize pthread mutex.");
      goto EXIT;
    }
    if ((rc = pthread_cond_init(&pt_cond, NULL)) != 0) {
      printf("Failed to initialize pthread condition variable.");
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

    printf("\nFIFO will be accessed as user: %s, group: %s\n\n", pw->pw_name, gr->gr_name);

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
     * Create victim FIFO.
     */
    if ((rc = mkfifo(fifoname, 0)) < 0) {
        goto EXIT;
    }

    /*
     * Test owner access modes
     */
    if ((rc = chown(fifoname, uid, gid_nobody)) < 0) {
        goto EXIT;
    }
    if ((rc = access_check(user_access)) < 0) {
        goto EXIT;
    }

    /*
     * Test group owner access modes
     */
    if ((rc = chown(fifoname, uid_nobody, gid)) < 0) {
        goto EXIT;
    }
    if ((rc = access_check(group_access)) < 0) {
        goto EXIT;
    }

    /*
     * Test other access modes
     */
    if ((rc = chown(fifoname, uid_nobody, gid_nobody)) < 0) {
        goto EXIT;
    }
    if ((rc = access_check(other_access)) < 0) {
        goto EXIT;
    }

    rc = 0;

EXIT:
    unlink(fifoname);
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
