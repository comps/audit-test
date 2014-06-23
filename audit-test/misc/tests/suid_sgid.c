#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <fcntl.h>
#include <pwd.h>
#include <grp.h>
#include <sys/types.h>
#include <sys/stat.h>

static char* filename = "./id";
static char* output   = "/tmp/output";
struct passwd *pw;
struct group *gr;

int g_rc  = 0;

/*
 * This is a cheap way to display the file
 * permissions and owner information.
 */
void fileinfo() {

    int fd;
    char readbuf[80];
    char lscommand[80];

    memset(readbuf, '\0', sizeof(readbuf));
    sprintf(lscommand, "ls -l %s >%s", filename, output);
    system(lscommand);

    fd = open(output, O_RDONLY);
    read(fd, readbuf, sizeof(readbuf));
    printf(readbuf);
    printf("\n");
    close(fd);
    unlink(output);
}

/*
 * Perform read, write and execute checks.
 */
int check_id(mode_t mode, char *uname, char *gname, int nobody) {

    int rc = 0;
    int fd;
    char result[20];


    if ((rc = chmod(filename, mode | S_IRWXU | S_IRWXG | S_IRWXO)) == -1) {
        goto EXIT;
    }

    fileinfo();

    /*
     * Check effective user
     */
    if (nobody) {
        if ((rc = system("su -s /bin/bash -c './id -un >/tmp/output' nobody")) == -1) {
          goto EXIT;
        }
    }
    else {
        if ((rc = system("./id -un >/tmp/output")) == -1) {
          goto EXIT;
        }
    }
    memset(result, '\0', sizeof(result));
    if ((fd = open(output, O_RDONLY)) == -1) {
      rc = -1;
      goto EXIT;
    }
    if ((rc = read(fd, result, strlen(uname))) == -1) {
      goto EXIT;
    }
    close(fd);
    if ((rc = strncmp(result, uname, strlen(uname))) != 0) {
      printf("expected user:  %s | actual user:  %s | FAIL\n", uname, result);
      g_rc = -1;
    } else {
      printf("expected user:  %s | actual user:  %s | PASS\n", uname, result);
    }

    /*
     * Check effective group
     */
    if (nobody) {
         if ((rc = system("su -s /bin/bash -c './id -gn >/tmp/output' nobody")) == -1) {
          goto EXIT;
        }
    }
    else {
        if ((rc = system("./id -gn >/tmp/output")) == -1) {
          goto EXIT;
        }
    }
    memset(result, '\0', sizeof(result));
    if ((fd = open(output, O_RDONLY)) == -1) {
      rc = -1;
      goto EXIT;
    }
    if ((rc = read(fd, result, strlen(gname))) == -1) {
      goto EXIT;
    }
    close(fd);
    if ((rc = strncmp(result, gname, strlen(gname))) != 0) {
      printf("expected group: %s | actual group: %s | FAIL\n", gname, result);
      g_rc = -1;
    } else {
      printf("expected group: %s | actual group: %s | PASS\n", gname, result);
    }
    printf("\n");

EXIT:
    unlink(output);

    return (rc);
}

int main(int argc, char *argv[]) {

    int rc = 0;

    uid_t uid_nobody;
    gid_t gid_nobody;

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
     * Create local id file - nobody:nobody
     */
    system("cp /usr/bin/id .");
    if ((rc = chown(filename, uid_nobody, gid_nobody)) == -1) {
        goto EXIT;
    }

    /*
     * Test suid - set suid bit, owner=nobody, group=root
     */
    check_id(S_ISUID, "nobody", "root", 0);

    /*
     * Test sgid - set sgid bit, owner=root, group=nobody
     */
    check_id(S_ISGID, "root", "nobody", 0);

    /*
     * Test suid/sgid - set suid/sgid bits owner=nobody, group=nobody
     */
    check_id(S_ISUID | S_ISGID, "nobody", "nobody", 0);

    /*
     * Change owner of local id file - root:root
     */
    if ((rc = chown(filename, 0, 0)) == -1) {
        goto EXIT;
    }

    /*
     * Test suid - set suid bit, owner=root, group=nobody
     */
    check_id(S_ISUID, "root", "nobody", 1);

    /*
     * Test sgid - set sgid bit, owner=nobody, group=root
     */
    check_id(S_ISGID, "nobody", "root", 1);

    /*
     * Test suid/sgid - set suid/sgid bits owner=root, group=root
     */
    check_id(S_ISUID | S_ISGID, "root", "root", 1);

EXIT:
    unlink(filename);

    rc = 0;
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
