/**********************************************************************
**   Copyright (C) International Business Machines  Corp., 2004
**
**   This program is free software;  you can redistribute it and/or modify
**   it under the terms of the GNU General Public License as published by
**   the Free Software Foundation; either version 2 of the License, or
**   (at your option) any later version.
**
**   This program is distributed in the hope that it will be useful,
**   but WITHOUT ANY WARRANTY;  without even the implied warranty of
**   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See
**   the GNU General Public License for more details.
**
**   You should have received a copy of the GNU General Public License
**   along with this program;  if not, write to the Free Software
**   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
**
**
**
**  FILE   : testfileperms.c
**
**  PURPOSE: The purpose of this test is to verify the file permissions
**           of all files under a given directory. The test makes 2 passes
**           through all the files.
**           Pass 1:
**                  Using the existing file attributes, verify file
**                  access as the file owner, group owner and other.
**                  The results of "open" are used to determine access.
**                  If the file owner is root, it is expected that access
**                  will be granted even if permissions are explicitly
**                  denied.
**           Pass 2:
**                  An attempt is made to chown the file to the provided
**                  testuser and testgroup.
**                  If the chown fails, a message is logged and the file
**                  is skipped.
**                  If the chown succeeds, a stat is performed to verify
**                  the chown was effective. If the file owner and group
**                  has not been modified, a message is logged and the file
**                  is skipped.
**                  Once the file is chowned, file permissions are verified
**                  as the testuser/testgroup.
**
**           In all cases, links are skipped.
**
**
**  HISTORY:
**    10/04 originated by Dan Jones (danjones@us.ibm.com)
**
**********************************************************************/
#include <sys/types.h>
#include <sys/dir.h>
#include <sys/param.h>
#include <sys/stat.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <pwd.h>
#include <grp.h>
#include <fcntl.h>
#include <errno.h>
#include <stdlib.h>

extern int alphasort ();

static int perms[] = {S_IRUSR, S_IWUSR, S_IXUSR,
                      S_IRGRP, S_IWGRP, S_IXGRP,
                      S_IROTH, S_IWOTH, S_IXOTH};
static char *ptext[] = {"r", "w", "x", "r", "w", "x", "r", "w", "x"};

int totalpass = 0;
int totalfail = 0;
int totalskip = 0;
uid_t uid_nobody = 65534;
gid_t gid_nobody = 65533;

int test = 0;
static char *test_description[] = {"Check default permissions",
				   "chown files to testuser/testgroup"};

/*
 * Do not include . or .. in directory list.
 */
int file_select (const struct direct *entry)
{
  if ((strcmp (entry->d_name, ".") == 0)
      || (strcmp (entry->d_name, "..") == 0))
    return (0);
  else
    return (1);
}

/*
 * Set euid, egid
 */
void setids(uid_t uid, gid_t gid) {

  int rc = 0;

  if ((rc = setegid(gid)) == -1)
    printf("\nERROR: unable to set gid. errno = %d\n", errno);
  if ((rc = seteuid(uid)) == -1)
    printf("\nERROR: unable to set uid. errno = %d\n", errno);

  return;
}

/*
 * Check actual vs. expected access using open system call
 */
void testaccess(char *pathname, int mode, int expected, char *outbuf) {

  int testrc = 0;
  int myerr = 0;

  if (expected == -1) {
    strcat(outbuf, "expected: fail  ");
  } else {
    strcat(outbuf, "expected: pass  ");
  }

  if ((testrc = open(pathname, mode)) == -1) {
    myerr = errno;
    strcat(outbuf, "actual: fail");
  } else {
    strcat(outbuf, "actual: pass");
    close(testrc);
  }

  if (myerr == ENODEV) {
    sprintf(&(outbuf[strlen(outbuf)]), "\tresult: SKIP : no device : %s\n", pathname);
    totalskip++;
  } else if (myerr == EBUSY) {
    sprintf(&(outbuf[strlen(outbuf)]), "\tresult: SKIP : device busy : %s\n", pathname);
    totalskip++;
  } else if (expected == 0) {
    if (testrc != -1)
      strcat(outbuf, "\tresult: PASS\n");
    else
      strcat(outbuf, "\tresult: PASS (MORE RESTRICTIVE)\n");
    totalpass++;
  } else if ((expected == -1) && (testrc == -1)) {
    strcat(outbuf, "\tresult: PASS\n");
    totalpass++;
  } else {
    sprintf(&(outbuf[strlen(outbuf)]), "\tresult: FAIL : errno = %d : %s\n", errno, pathname);
    totalfail++;
  }

  printf("%s", outbuf);

  return;
}

/*
 * Test access for owner, group, other
 */
void testall(struct stat *ostatbufp, char *pathname, uid_t uid, gid_t gid) {

  int i;
  int rc = 0;
  char outbuf[256];
  struct passwd passwd;
  struct passwd *passwdp;
  struct group group;
  struct group *groupp;
  struct stat statbuf;
  struct stat *statbufp = &statbuf;
  char *pbuf;
  char *gbuf;

  passwdp = (struct passwd *)malloc(sizeof(passwd));
  groupp  = (struct group *)malloc(sizeof(group));
  pbuf = (char *)malloc(4096);
  gbuf = (char *)malloc(4096);

  if (!passwdp || !groupp || !pbuf || !gbuf) {
    printf("ERROR: unable to allocate enough memory\n");
    goto EXIT;
  }

  *pbuf = '\0';
  *gbuf = '\0';

  setids(0,0);
  printf("\n%s\n", pathname);

  // For test 1 we chown the file owner/group
  if (test == 1) {
    if ((rc = chown(pathname, uid, gid)) == -1) {
      printf("ERROR: unable to chown %s to %d:%d\n", pathname, uid, gid);
      goto EXIT;
    }
  }

  // Start with clean buffers
  memset(&statbuf, '\0', sizeof(statbuf));

  // Get file stat info to determine actual owner and group
  stat(pathname, &statbuf);

  // If we successfully chow'd the file, but the owner hasn't changed
  // log it and skip.
  if ((test == 1) && ((statbufp->st_uid != uid) || (statbufp->st_gid != gid))) {
    printf("INFO: chown success, but file owner did not change: %s\n", pathname);
    totalskip++;
    goto EXIT;
  }

  strcpy(outbuf, "MODE: ");
  for (i = 0; i < sizeof(perms)/sizeof(int); i++) {
    if (statbufp->st_mode & perms[i]) {
      strcat(outbuf, ptext[i]);
    } else {
      strcat(outbuf, "-");
    }
  }
  getpwuid_r(statbufp->st_uid, &passwd, pbuf, 4096, &passwdp);
  getgrgid_r(statbufp->st_gid, &group, gbuf, 4096, &groupp);

  sprintf(&(outbuf[strlen(outbuf)]), "  %s:%s\n", passwd.pw_name, group.gr_name);
  printf("%s", outbuf);

  // Check owner access for read/write
  setids(statbufp->st_uid, gid_nobody);
  strcpy(outbuf, "Owner read\t");
  // If we are root, we expect to succeed event
  // without explicit permission.
  if ((statbufp->st_mode & S_IRUSR) || (statbufp->st_uid == 0)) {
    testaccess(pathname, O_RDONLY, 0, outbuf);
  } else {
    testaccess(pathname, O_RDONLY, -1, outbuf);
  }
  strcpy(outbuf, "Owner write\t");
  // If we are root, we expect to succeed event
  // without explicit permission.
  if ((statbufp->st_mode & S_IWUSR) || (statbufp->st_uid == 0)) {
    testaccess(pathname, O_WRONLY, 0, outbuf);
  } else {
    testaccess(pathname, O_WRONLY, -1, outbuf);
  }

  // Check group access for read/write
  setids(0, 0);
  setids(uid_nobody, statbufp->st_gid);
  strcpy(outbuf, "Group read\t");
  if (statbufp->st_mode & S_IRGRP) {
    testaccess(pathname, O_RDONLY, 0, outbuf);
  } else {
    testaccess(pathname, O_RDONLY, -1, outbuf);
  }
  strcpy(outbuf, "Group write\t");
  if (statbufp->st_mode & S_IWGRP) {
    testaccess(pathname, O_WRONLY, 0, outbuf);
  } else {
    testaccess(pathname, O_WRONLY, -1, outbuf);
  }

  // Check other access for read/write
  setids(0, 0);
  setids(uid_nobody, gid_nobody);
  strcpy(outbuf, "Other read\t");
  if (statbufp->st_mode & S_IROTH) {
    testaccess(pathname, O_RDONLY, 0, outbuf);
  } else {
    testaccess(pathname, O_RDONLY, -1, outbuf);
  }
  strcpy(outbuf, "Other write\t");
  if (statbufp->st_mode & S_IWOTH) {
    testaccess(pathname, O_WRONLY, 0, outbuf);
  } else {
    testaccess(pathname, O_WRONLY, -1, outbuf);
  }

  setids(0, 0);

  if (test == 1) {
    chown(pathname, ostatbufp->st_uid, ostatbufp->st_gid);
  }

EXIT:

  return;
}

/*
 * Check access.
 *
 * This method check a file or recursively scan directories and verify
 * the file access modes are enforced.
 *
 */
void check_access (char *pathname, uid_t uid, gid_t gid)
{
  int count = 0;
  int i = 0;
  int rc = 0;
  int file_select ();
  char entry[MAXPATHLEN];
  struct dirent **entries;
  struct stat statbuf;

  // Reset our extended group list
  setgroups(0, NULL);

  // Start with clean buffers
  memset(&statbuf, '\0', sizeof(statbuf));

  // Get file stat info.
  if ((rc = lstat(pathname, &statbuf)) == -1) {
    printf("\nERROR: %s. Could not obtain file status. errno = %d\n", pathname, errno);
    goto EXIT;
  }

  // If link, skip it.
  if (S_ISLNK(statbuf.st_mode)) {
    printf("Link: skipping %s\n", entry);
    totalskip++;
    goto EXIT;
  }

  // If not a directory, check it and leave.
  if (!(S_ISDIR(statbuf.st_mode))) {
    testall(&statbuf, pathname, uid, gid);
    goto EXIT;
  }

  // If directory, recurse through all subdirectories, checking all files.
  if ((count = scandir (pathname, &entries, file_select, alphasort)) == -1) {
    printf("\nERROR: %s. Could not scandir. errno = %d\n", pathname, errno);
    goto EXIT;
  }
  for (i = 0; i < count; i++) {

    sprintf(entry, "%s/%s", pathname, entries[i]->d_name);

    // If link, skip it
    // Else if directory, call check_access() recursively
    if (entries[i]->d_type == DT_LNK) {
      printf("Link: skipping %s\n", entry);
      totalskip++;
      continue;
    } else if (entries[i]->d_type == DT_DIR) {
      check_access(entry, uid, gid);
      continue;
    }

    // Clean the buffer
    memset(&statbuf, '\0', sizeof(statbuf));

    // Get file stat info.
    if ((rc = lstat(entry, &statbuf)) == -1) {
      printf("\nERROR: %s. Could not obtain file status. errno = %d\n", pathname, errno);
      continue;
    }

    // The directory entry doesn't always seem to have the
    // right info. So we check again after the stat().
    //
    // If link, skip it
    // Else if directory, call check_access() recursively
    // Else check access
    if (S_ISLNK(statbuf.st_mode)) {
      printf("Link: (2) skipping %s\n", entry);
      totalskip++;
      continue;
    } else if (S_ISDIR(statbuf.st_mode)) {
      check_access(entry, uid, gid);
      continue;
    } else {
      testall(&statbuf, entry, uid, gid);
      continue;
    }
  }

EXIT:

  return;
}

int main (int argc, char *argv[]) {

  int i = 0;
  struct passwd *pw;
  struct group *gr;

  if (argc != 4) {
    printf("usage: %s <directory> <testuser> <testgroup>\n", argv[0]);
    goto EXIT;
  }

  if ((pw = getpwnam(argv[2])) == NULL) {
    printf("ERROR: invalid username %s\n", argv[2]);
    goto EXIT;
  }
  if ((gr = getgrnam(argv[3])) == NULL) {
    printf("ERROR: invalid group %s\n", argv[3]);
    goto EXIT;
  }

  for (i = 0; i < 2; i++) {
    totalpass = 0;
    totalfail = 0;
    test = i;
    printf("Test: %s\n\n", test_description[i]);
    check_access(argv[1], pw->pw_uid, gr->gr_gid);
    printf("\nTEST PASSED = %d, FAILED = %d, SKIPPED = %d\n", totalpass, totalfail, totalskip);
  }

 EXIT:

  return (0);
}
