#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <pwd.h>
#include <grp.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/shm.h>
#include <sys/ipc.h>
#include <errno.h>

mode_t user_access[]  = {S_IRUSR, S_IWUSR, S_IRUSR | S_IWUSR};
mode_t group_access[] = {S_IRGRP, S_IWGRP, S_IRGRP | S_IWGRP};
mode_t other_access[] = {S_IROTH, S_IWOTH, S_IROTH | S_IWOTH};

static int READ = 0;
static int WRITE = 1;
static int READWRITE = 2;

uid_t uid = 0;
gid_t gid = 0;
int shmid = 0;
int g_rc = 0;

/*
 * Display permission bits and owner info.
 */
void shminfo ()
{
  int rc = 0;
  struct passwd *pw;
  struct group *gr;
  struct shmid_ds buf;

  shmctl (shmid, IPC_STAT, &buf);
  pw = getpwuid (buf.shm_perm.uid);
  gr = getgrgid (buf.shm_perm.gid);

  if ((buf.shm_perm.mode & user_access[READ]) != 0)
    printf("R");
  else
    printf("-");

  if ((buf.shm_perm.mode & user_access[WRITE]) != 0)
    printf("W");
  else
    printf("-");
  printf("-");

  if ((buf.shm_perm.mode & group_access[READ]) != 0)
    printf("R");
  else
    printf("-");

  if ((buf.shm_perm.mode & group_access[WRITE]) != 0)
    printf("W");
  else
    printf("-");
  printf("-");

  if ((buf.shm_perm.mode & other_access[READ]) != 0)
    printf("R");
  else
    printf("-");

  if ((buf.shm_perm.mode & other_access[WRITE]) != 0)
    printf("W");
  else
    printf("-");
  printf("-");

  printf(" Owner = %s, Group = %s\n", pw->pw_name, gr->gr_name);
  return;
}



/*
 * Perform read checks.
 */
int access_check (mode_t access[])
{
  int i;
  int rc = 0;
  char result[80];
  char *return_string;
  struct shmid_ds buf;
  char *shmPtr;

  for (i = 0; i < 3; i++) {

    // Set permission mode for queue.
    if ((rc = shmctl (shmid, IPC_STAT, &buf)) == -1) {
      goto EXIT;
    }
    buf.shm_perm.mode = access[i];
    if ((rc = shmctl (shmid, IPC_SET, &buf)) == -1) {
      goto EXIT;
    }

    // Display current permission/owner info.
    shminfo ();

    // Substitute user, group.
    if ((rc = setegid (gid) == -1)) {
      perror ("setegid");
      goto EXIT;
    }
    if ((rc = seteuid (uid) == -1)) {
      perror ("seteuid");
      goto EXIT;
    }

    // Try read only access.
    return_string = strcpy (result, "Access:READ     ");
    shmPtr = shmat (shmid, NULL, SHM_RDONLY);
    if (shmPtr == (char *) -1) {
      return_string = strcat (result, " | Allowed:NO ");
    } else {
      return_string = strcat (result, " | Allowed:YES");
    }
    if (((shmPtr != (char *) -1) && (access[i] == access[READ])) ||
	  ((shmPtr != (char *) -1) && (access[i] == access[READWRITE])) ||
	  ((shmPtr == (char *) -1) && (access[i] != access[READ]))) {
      return_string = strcat (result, " | PASS\n");
    } else {
      return_string = strcat (result, " | FAIL\n");
      g_rc = -1;
    }
    printf (result);

    // Try read/write access.
    return_string = strcpy (result, "Access:READWRITE");
    shmPtr = shmat (shmid, NULL, 0);
    if (shmPtr == (char *) -1) {
      return_string = strcat (result, " | Allowed:NO ");
    } else {
      return_string = strcat (result, " | Allowed:YES");
    }
    if ((shmPtr != (char *) -1) && (access[i] == access[READWRITE]) ||
	  (shmPtr == (char *) -1) && (access[i] != access[READWRITE])) {
      return_string = strcat (result, " | PASS\n");
    } else {
      return_string = strcat (result, " | FAIL\n");
      g_rc = -1;
    }
    printf (result);
    printf ("\n");

    // Reset uid/gid
    if ((rc = seteuid (0)) == -1) {
      goto EXIT;
    }
    if ((rc = setegid (0)) == -1) {
      goto EXIT;
    }
  }
  rc = 0;

EXIT:

  return (rc);
}
int main (int argc, char *argv[])
{
  int rc = 0;

  key_t key;
  char *shmPtr;

  uid_t uid_nobody;
  gid_t gid_nobody;
  uid_t uid_shmcreator;
  gid_t gid_shmcreator;
  struct passwd *pw;
  struct group *gr;
  struct shmid_ds buf;

  if (argc != 3) {
    printf ("Please provide username.\n");
    rc = -1;
    goto EXIT;
  }
  if ((pw = getpwnam (argv[1])) == NULL) {
    printf ("Invalid username.\n");
    rc = -1;
    goto EXIT;
  }
  if ((gr = getgrgid (pw->pw_gid)) == NULL) {
    printf ("Invalid group.\n");
    rc = -1;
    goto EXIT;
  }
  uid = pw->pw_uid;
  gid = gr->gr_gid;
  printf ("\nshm will be accessed as user: %s, group: %s\n", pw->pw_name,
	    gr->gr_name);

  /*
   * Get shm creator user name/group.
   */
  if ((pw = getpwnam (argv[2])) == NULL) {
    printf ("Invalid shm creator name.\n");
    rc = -1;
    goto EXIT;
  }
  if ((gr = getgrgid (pw->pw_gid)) == NULL) {
    printf ("Invalid shm creator group.\n");
    rc = -1;
    goto EXIT;
  }
  uid_shmcreator = pw->pw_uid;
  gid_shmcreator = gr->gr_gid;
  printf ("\nshm will be created as user: %s, group: %s\n", pw->pw_name,
	    gr->gr_name);

  /*
   * Get uid/gid for nobody
   */
  if ((pw = getpwnam ("nobody")) == NULL) {
    printf ("Invalid username.\n");
    rc = -1;
    goto EXIT;
  }
  uid_nobody = pw->pw_uid;
  if ((gr = getgrnam ("nobody")) == NULL) {
    printf ("Invalid group.\n");
    rc = -1;
    goto EXIT;
  }
  gid_nobody = gr->gr_gid;

  /*
   * Create victim shared memory segment
   */
  setegid (gid_shmcreator);
  seteuid (uid_shmcreator);
  if ((shmid = shmget (IPC_PRIVATE, 1, S_IRUSR)) == -1) {
    printf ("\nError in shmget\n\n");
    goto EXIT;
  }
  seteuid (0);
  setegid (0);

  /*
   * Attach recently created shared memory segment shmid to this process
   */
  if ((rc = shmctl (shmid, IPC_STAT, &buf)) == -1) {
    perror ("shmctl");
    printf ("\nError in IPC_STAT shmctl\n\n");
    goto EXIT;
  }

  /*
   * Test owner access modes
   */
  buf.shm_perm.uid = uid;
  buf.shm_perm.gid = gid_nobody;
  if ((rc = shmctl (shmid, IPC_SET, &buf)) == -1) {
    printf ("\nError in IPC_SET shmctl uid_nobody\n\n");
    goto EXIT;
  }
  if ((rc = access_check (user_access)) == -1) {
    printf ("\nError in access_check user_access \n\n");
    goto EXIT;
  }

  /*
   * Test group owner access modes
   */
  buf.shm_perm.uid = uid_nobody;
  buf.shm_perm.gid = gid;
  if ((rc = shmctl (shmid, IPC_SET, &buf)) == -1) {
    printf ("\nError in IPC_SET shmctl gid_nobody \n\n");
    goto EXIT;
  }
  if ((rc = access_check (group_access)) == -1) {
    printf ("\nError in access_check group_access \n\n");
    goto EXIT;
  }

  /*
   * Test other access modes: current oid is oid
   */
  buf.shm_perm.uid = uid_nobody;
  buf.shm_perm.gid = gid_nobody;
  if ((rc = shmctl (shmid, IPC_SET, &buf)) == -1) {
    printf ("\nError in IPC_SET shmctl gid_nobody and uid_nobody \n\n");
    goto EXIT;
  }
  if ((rc = access_check (other_access)) == -1) {
    printf ("\nError in access_check other_access\n\n");
    goto EXIT;
  }
  rc = 0;

EXIT:

  /*
   * Remove message queue.
   */
  shmctl(shmid, IPC_RMID, NULL);


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
