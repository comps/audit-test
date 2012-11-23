#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <fcntl.h>
#include <pwd.h>
#include <grp.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/types.h>
#include <sys/stat.h>

mode_t user_access[]  = {S_IRUSR, S_IWUSR, 0};
mode_t group_access[] = {S_IRGRP, S_IWGRP, 0};
mode_t other_access[] = {S_IROTH, S_IWOTH, 0};

static int READ     = 0;
static int WRITE    = 1;
static int NOACCESS = 2;

uid_t uid = 0;
gid_t gid = 0;

int semid = 0;
int g_rc  = 0;

union semun {
	int val;
	struct semid_ds *buf;
	unsigned short int *array;
	struct seminfo *__buf;
};

/*
 * Display permission bits and owner info.
 */
void seminfo()
{
  int rc = 0;
  char output[80];
  char access[10];
  char *return_string;

  struct passwd *pw;
  struct group *gr;
  struct semid_ds semstat;
  union semun arg;

  arg.buf = &semstat;

  memset(output, '\0', sizeof(output));
  memset(access, '\0', sizeof(access));

  semctl(semid, 0, IPC_STAT, arg);
  pw = getpwuid(semstat.sem_perm.uid);
  gr = getgrgid(semstat.sem_perm.gid);

  if ((semstat.sem_perm.mode & user_access[READ]) != 0) {
    return_string = strcat(access, "R");
  } else {
    return_string = strcat(access, "-");
  }
  if ((semstat.sem_perm.mode & user_access[WRITE]) != 0) {
    return_string = strcat(access, "W");
  } else {
    return_string = strcat(access, "-");
  }
  return_string = strcat(access, "-");

  if ((semstat.sem_perm.mode & group_access[READ]) != 0) {
    return_string = strcat(access, "R");
  } else {
    return_string = strcat(access, "-");
  }
  if ((semstat.sem_perm.mode & group_access[WRITE]) != 0) {
    return_string = strcat(access, "W");
  } else {
    return_string = strcat(access, "-");
  }
  return_string = strcat(access, "-");

  if ((semstat.sem_perm.mode & other_access[READ]) != 0) {
    return_string = strcat(access, "R");
  } else {
    return_string = strcat(access, "-");
  }
  if ((semstat.sem_perm.mode & other_access[WRITE]) != 0) {
    return_string = strcat(access, "W");
  } else {
    return_string = strcat(access, "-");
  }

  return_string = strcat(access, "-");
  return_string = strcat(output, access);
  return_string = strcat(output, " Owner = ");
  return_string = strcat(output, pw->pw_name);
  return_string = strcat(output, ", Group = ");
  return_string = strcat(output, gr->gr_name);
  printf(output);
  printf("\n");
}

/*
 * Perform read and write checks.
 */
int access_check(mode_t access[])
{
  int i;
  int rc = 0;
  ssize_t s;
  char result[80];
  char *return_string;
  struct semid_ds semstat;
  struct sembuf sop;
  union semun arg;

  arg.buf = &semstat;

  for (i = 0; i < 3; i++) {

    // Set permission mode for queue.
    if ((rc = semctl(semid, 0, IPC_STAT, arg)) == -1) {
      goto EXIT;
    }
    semstat.sem_perm.mode = access[i];
    if ((rc = semctl(semid, 0, IPC_SET, arg)) == -1) {
      goto EXIT;
    }

    // Display current permission/owner info.
    seminfo();

    // Substitute user, group.
    if ((rc = setegid(gid)) == -1) {
      goto EXIT;
    }
    if ((rc = seteuid(uid)) == -1) {
      goto EXIT;
    }

    // Try read access (sem_op = 0).
    return_string = strcpy(result, "Access:READ ");
    sop.sem_num = 0;
    sop.sem_op  = 0; // read access
    sop.sem_flg = SEM_UNDO;
    rc = semop(semid, &sop, 1);
    if (rc == -1) {
      return_string = strcat(result, " | Allowed:NO ");
    } else {
      return_string = strcat(result, " | Allowed:YES");
    }
    if ((rc != -1) && (access[i] == access[READ]) ||
	(rc == -1) && (access[i] != access[READ])) {
      return_string = strcat(result, " | PASS\n");
    } else {
      return_string = strcat(result, " | FAIL\n");
      g_rc = -1;
    }
    printf(result);

    // Try write access (sem_op = 1)
    return_string = strcpy(result, "Access:WRITE");
    sop.sem_num = 0;
    sop.sem_op  = 1; // write access
    sop.sem_flg = SEM_UNDO;
    rc = semop(semid, &sop, 1);
    if (rc == -1) {
      return_string = strcat(result, " | Allowed:NO ");
    } else {
      return_string = strcat(result, " | Allowed:YES");
      // Reset the semaphore
      sop.sem_num = 0;
      sop.sem_op  = -1; // write access
      sop.sem_flg = SEM_UNDO;
      semop(semid, &sop, 1);
    }
    if ((rc != -1) && (access[i] == access[WRITE]) ||
	(rc == -1) && (access[i] != access[WRITE])) {
      return_string = strcat(result, " | PASS\n");
    } else {
      return_string = strcat(result, " | FAIL\n");
      g_rc = -1;
    }
    printf(result);
    printf("\n");

    // Reset uid/gid
    if ((rc = seteuid(0)) == -1) {
      goto EXIT;
    }
    if ((rc = setegid(0)) == -1) {
      goto EXIT;
    }
  }

EXIT:

  return (rc);
}

int main(int argc, char *argv[])
{
  int fd;
  int rc = 0;

  uid_t uid_nobody;
  gid_t gid_nobody;
  uid_t uid_screator;
  gid_t gid_screator;

  struct passwd *pw;
  struct group *gr;
  struct semid_ds semstat;
  union semun arg;

  arg.buf = &semstat;

  if (argc != 3) {
    printf("Please provide test user name and semaphore creator name.\n");
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

  printf("\nSemaphore will be accessed as user: %s, group: %s\n",
	 pw->pw_name, gr->gr_name);

  /*
   * Get semaphore creator user name/group.
   */
  if ((pw = getpwnam(argv[2])) == NULL) {
    printf("Invalid semaphore creator name.\n");
    rc = -1;
    goto EXIT;
  }
  if ((gr = getgrgid(pw->pw_gid)) == NULL) {
    printf("Invalid semaphore creator group.\n");
    rc = -1;
    goto EXIT;
  }

  uid_screator = pw->pw_uid;
  gid_screator = gr->gr_gid;

  printf("Semaphore will be created as user: %s, group: %s\n\n",
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
   * Create the message queue.
   */
  setegid(gid_screator);
  seteuid(uid_screator);
  if ((semid = semget(IPC_PRIVATE, 1, S_IRUSR)) == -1) {
    goto EXIT;
  }
  seteuid(0);
  setegid(0);

  /*
   * Test owner access modes
   */
  if ((rc = semctl(semid, 0, IPC_STAT, arg)) == -1) {
    goto EXIT;
  }
  semstat.sem_perm.uid = uid;
  semstat.sem_perm.gid = gid_nobody;
  if ((rc = semctl(semid, 0, IPC_SET, arg)) == -1) {
    goto EXIT;
  }
  if ((rc = access_check(user_access)) == -1) {
    goto EXIT;
  }

  /*
   * Test group owner access modes
   */
  if ((rc = semctl(semid, 0, IPC_STAT, arg)) == -1) {
    goto EXIT;
  }
  semstat.sem_perm.uid = uid_nobody;
  semstat.sem_perm.gid = gid;
  if ((rc = semctl(semid, 0, IPC_SET, arg)) == -1) {
    goto EXIT;
  }
  if ((rc = access_check(group_access)) == -1) {
    goto EXIT;
  }

  /*
   * Test other access modes
   */
  if ((rc = semctl(semid, 0, IPC_STAT, arg)) == -1) {
    goto EXIT;
  }
  semstat.sem_perm.uid = uid_nobody;
  semstat.sem_perm.gid = gid_nobody;
  if ((rc = semctl(semid, 0, IPC_SET, arg)) == -1) {
    goto EXIT;
  }
  if ((rc = access_check(other_access)) == -1) {
    goto EXIT;
  }

EXIT:

  /*
   * Remove message queue.
   */
  semctl(semid, 0, IPC_RMID, NULL);

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
