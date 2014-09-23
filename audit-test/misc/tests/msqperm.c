#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <fcntl.h>
#include <pwd.h>
#include <grp.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/types.h>
#include <sys/stat.h>

mode_t user_access[]  = {S_IRUSR, S_IWUSR};
mode_t group_access[] = {S_IRGRP, S_IWGRP};
mode_t other_access[] = {S_IROTH, S_IWOTH};

static int READ = 0;
static int WRITE = 1;

uid_t uid = 0;
gid_t gid = 0;

int msqid = 0;
int g_rc  = 0;

/*
 * Display permission bits and owner info.
 */
void msqinfo()
{
  struct passwd *pw;
  struct group *gr;
  struct msqid_ds msqstat;

  msgctl(msqid, IPC_STAT, &msqstat);
  pw = getpwuid(msqstat.msg_perm.uid);
  gr = getgrgid(msqstat.msg_perm.gid);

  if ((msqstat.msg_perm.mode & user_access[READ]) != 0) {
    printf("R");
  } else {
    printf("-");
  }

  if ((msqstat.msg_perm.mode & user_access[WRITE]) != 0) {
    printf("W");
  } else {
    printf("-");
  }
  printf("-");

  if ((msqstat.msg_perm.mode & group_access[READ]) != 0) {
    printf("R");
  } else {
    printf("-");
  }

  if ((msqstat.msg_perm.mode & group_access[WRITE]) != 0) {
    printf("W");
  } else {
    printf("-");
  }
  printf("-");

  if ((msqstat.msg_perm.mode & other_access[READ]) != 0) {
    printf("R");
  } else {
    printf("-");
  }

  if ((msqstat.msg_perm.mode & other_access[WRITE]) != 0) {
    printf("W");
  } else {
    printf("-");
  }

  printf(" Owner = %s, Group = %s\n", pw->pw_name, gr->gr_name);
  return;
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
  struct msqid_ds msqstat;
  struct msgbuf
  {
    long mtype;
    char mtext[80];
  } r_message, s_message;

  for (i = 0; i < 2; i++) {

    // Set permission mode for queue.
    if ((rc = msgctl(msqid, IPC_STAT, &msqstat)) == -1) {
      goto EXIT;
    }
    msqstat.msg_perm.mode = access[i];
    msqstat.msg_qnum = 0;
    if ((rc = msgctl(msqid, IPC_SET, &msqstat)) == -1) {
      goto EXIT;
    }

    // First prime the msg queue so read doesn't fail.
    s_message.mtype = 1;
    memset(s_message.mtext, '\0', sizeof(s_message.mtext));
    strcpy(s_message.mtext, "Prime Message\0");
    if ((rc = msgsnd(msqid, &s_message, strlen(s_message.mtext), 0)) == -1) {
      goto EXIT;
    }

    // Display current permission/owner info.
    msqinfo();

    // Substitute user, group.
    if ((rc = setegid(gid)) == -1) {
      goto EXIT;
    }
    if ((rc = seteuid(uid)) == -1) {
      goto EXIT;
    }

    // Try read access.
    memset(r_message.mtext, '\0', sizeof(r_message.mtext));
    s = msgrcv(msqid, &r_message, sizeof(r_message.mtext), 0, IPC_NOWAIT);
    if ((s != -1 && access[i] == access[READ]) ||
	(s == -1 && access[i] != access[READ])) {
    } else {
      g_rc = -1;
    }
    printf(result);

    // Try write access
    s_message.mtype = 1;
    memset(s_message.mtext, '\0', sizeof(s_message.mtext));
    strcpy(s_message.mtext, "Send Test\0");
    rc = msgsnd(msqid, &s_message, sizeof(s_message.mtext), 0);
    if ((rc != -1 && access[i] == access[WRITE]) ||
	(rc == -1 && access[i] != access[WRITE])) {
    } else {
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
  int rc = 0;

  uid_t uid_nobody;
  gid_t gid_nobody;
  uid_t uid_qcreator;
  gid_t gid_qcreator;

  struct passwd *pw;
  struct group *gr;
  struct msqid_ds msqstat;

  if (argc != 3) {
    printf("Please provide test user name and queue creator name.\n");
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

  printf("\nFiles will be accessed as user: %s, group: %s\n",
	 pw->pw_name, gr->gr_name);

  /*
   * Get queue creator user name/group.
   */
  if ((pw = getpwnam(argv[2])) == NULL) {
    printf("Invalid queue creator name.\n");
    rc = -1;
    goto EXIT;
  }
  if ((gr = getgrgid(pw->pw_gid)) == NULL) {
    printf("Invalid queue creator group.\n");
    rc = -1;
    goto EXIT;
  }

  uid_qcreator = pw->pw_uid;
  gid_qcreator = gr->gr_gid;

  printf("Message queue will be created as user: %s, group: %s\n\n",
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
  setegid(gid_qcreator);
  seteuid(uid_qcreator);
  if ((msqid = msgget(IPC_PRIVATE, 0)) == -1) {
    goto EXIT;
  }
  seteuid(0);
  setegid(0);

  /*
   * Test owner access modes
   */
  if ((rc = msgctl(msqid, IPC_STAT, &msqstat)) == -1) {
    goto EXIT;
  }
  msqstat.msg_perm.uid = uid;
  msqstat.msg_perm.gid = gid_nobody;
  if ((rc = msgctl(msqid, IPC_SET, &msqstat)) == -1) {
    goto EXIT;
  }
  if ((rc = access_check(user_access)) == -1) {
    goto EXIT;
  }

  /*
   * Test group owner access modes
   */
  if ((rc = msgctl(msqid, IPC_STAT, &msqstat)) == -1) {
    goto EXIT;
  }
  msqstat.msg_perm.uid = uid_nobody;
  msqstat.msg_perm.gid = gid;
  if ((rc = msgctl(msqid, IPC_SET, &msqstat)) == -1) {
    goto EXIT;
  }
  if ((rc = access_check(group_access)) == -1) {
    goto EXIT;
  }

  /*
   * Test other access modes
   */
  if ((rc = msgctl(msqid, IPC_STAT, &msqstat)) == -1) {
    goto EXIT;
  }
  msqstat.msg_perm.uid = uid_nobody;
  msqstat.msg_perm.gid = gid_nobody;
  if ((rc = msgctl(msqid, IPC_SET, &msqstat)) == -1) {
    goto EXIT;
  }
  if ((rc = access_check(other_access)) == -1) {
    goto EXIT;
  }

EXIT:

  /*
   * Remove message queue.
   */
  msgctl(msqid, IPC_RMID, NULL);

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
