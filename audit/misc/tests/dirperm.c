#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <dirent.h>
#include <pwd.h>
#include <grp.h>
#include <sys/types.h>
#include <sys/stat.h>

/*
 * Directories must have "X" as well as "R" or "W" for access.
 */
mode_t user_access[]  = {S_IRUSR | S_IXUSR, S_IWUSR | S_IXUSR, S_IXUSR};
mode_t group_access[] = {S_IRGRP | S_IXGRP, S_IWGRP | S_IXGRP, S_IXGRP};
mode_t other_access[] = {S_IROTH | S_IXOTH, S_IWOTH | S_IXOTH, S_IXOTH};

static char* filename  = "./dactestdir/dactestfile";
static char* workdir   = "./workdir";
static char* dirname   = "./dactestdir";
static char* output    = "./output";
static char* searchdir = "ls -la ./dactestdir 2>/dev/null";
static int   READ   = 0;
static int   WRITE  = 1;
static int   SEARCH = 2;

struct passwd *pw;
struct group  *gr;

uid_t uid;
gid_t gid;

int g_rc = 0;

/*
 * This is a cheap way to display the file
 * permissions and owner information.
 */
void dirinfo() {

    int fd;
    char readbuf[80];
    char lscommand[80];

    memset(readbuf, '\0', sizeof(readbuf));
    memset(lscommand, '\0', sizeof(lscommand));

    sprintf(lscommand, "ls -ld %s >%s", dirname, output);
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
int access_check(mode_t access[]) {

    int i;
    int rc = 0;
    int fd;
    char result[80];
    char *return_string;
    DIR *dir;

    for (i = 0; i < 3; i++) {
        if ((rc = chmod(dirname, access[i])) == -1) {
            goto EXIT;
        }
        if ((rc = setegid(gid)) == -1) {
            goto EXIT;
        }
        if ((rc = seteuid(uid)) == -1) {
            goto EXIT;
        }

	memset(result, '\0', sizeof(result));

        // Display current permission/owner info
        dirinfo();

        // Try read access
        return_string = strcpy(result, "Access:READ  ");
        dir = opendir(dirname);
        if (dir == NULL) {
            return_string = strcat(result, " | Allowed:NO ");
        } else {
            return_string = strcat(result, " | Allowed:YES");
        }

        if ((dir != NULL) && (access[i] == access[READ]) ||
            (dir == NULL) && (access[i] != access[READ])) {
            return_string = strcat(result, " | PASS\n");
        } else {
            return_string = strcat(result, " | FAIL\n");
	    g_rc = -1;
        }

        printf(result);

        if (dir != NULL) {
            if ((rc == closedir(dir)) == -1) {
                goto EXIT;
            }
        }

        // Try write access
        return_string = strcpy(result, "Access:WRITE ");
        fd = creat(filename, O_WRONLY);
        if (fd == -1) {
            return_string = strcat(result, " | Allowed:NO ");
        } else {
            return_string = strcat(result, " | Allowed:YES");
        }

        if ((fd != -1) && (access[i] == access[WRITE]) ||
            (fd == -1) && (access[i] != access[WRITE])) {
            return_string = strcat(result, " | PASS\n");
        } else {
            return_string = strcat(result, " | FAIL\n");
	    g_rc = -1;
        }

        printf(result);

        if (fd != -1) {
            if ((rc == unlink(filename)) == -1) {
                goto EXIT;
            }
        }

        // Try cd
        return_string = strcpy(result, "Access:SEARCH");
	rc = chdir(dirname);
        if (rc == -1) {
            return_string = strcat(result, " | Allowed:NO ");
        } else {
            return_string = strcat(result, " | Allowed:YES");
        }

        if ((rc != -1) || (rc == -1) && (access[i] != access[SEARCH])) {
            return_string = strcat(result, " | PASS\n");
        } else {
            return_string = strcat(result, " | FAIL\n");
	    g_rc = -1;
        }

        printf(result);

        printf("\n");

	if (rc != -1) chdir("..");

        // Reset uid/gid
        if ((rc = seteuid(0)) == -1) {
            goto EXIT;
        }
        if ((rc = setegid(0)) == -1) {
            goto EXIT;
        }
    }

    rc = 0;

EXIT:

    return (rc);

}

int main(int argc, char *argv[]) {

    int fd;
    int rc = 0;
    int cd = 0;

    uid_t uid_nobody;
    gid_t gid_nobody;

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

    printf("\nFiles will be accessed as user: %s, group: %s\n\n", pw->pw_name, gr->gr_name);

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
     * Create a working directory with full access.
     */
    if ((rc = mkdir(workdir, O_CREAT)) == -1) {
        goto EXIT;
    }
    if ((rc = chmod(workdir,
		    S_IRUSR | S_IWUSR | S_IXUSR |
		    S_IRGRP | S_IWGRP | S_IXGRP |
		    S_IROTH | S_IWOTH | S_IXOTH)) == -1) {
        goto EXIT;
    }
    if ((rc = chdir(workdir)) == -1) {
        goto EXIT;
    }
    cd = 1;

    /*
     * Create victim directory.
     */
    if ((rc = mkdir(dirname, O_CREAT)) == -1) {
        goto EXIT;
    }

    /*
     * Test owner access modes
     */
    if ((rc = chown(dirname, uid, gid_nobody)) == -1) {
        goto EXIT;
    }

    if ((rc = access_check(user_access)) == -1) {
        goto EXIT;
    }

    /*
     * Test group owner access modes
     */
    if ((rc = chown(dirname, uid_nobody, gid)) == -1) {
        goto EXIT;
    }

    if ((rc = access_check(group_access)) == -1) {
        goto EXIT;
    }

    /*
     * Test other access modes
     */
    if ((rc = chown(dirname, uid_nobody, gid_nobody)) == -1) {
        goto EXIT;
    }

    if ((rc = access_check(other_access)) == -1) {
        goto EXIT;
    }

    rc = 0;

EXIT:

    rmdir(dirname);
    if (cd == 1) chdir("..");
    rmdir(workdir);

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
