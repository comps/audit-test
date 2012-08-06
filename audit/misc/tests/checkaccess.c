#include <sys/types.h>
#include <sys/dir.h>
#include <sys/param.h>
#include <sys/stat.h>
#include <stdio.h>
#include <unistd.h>

extern int alphasort ();

int g_rc = 0;

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
 * Check access.
 *
 * This method will recursively scan directories and check
 * that the file owner is root and that the file is not
 * writable by group or world.
 */
int check_access (char *pathname)
{
  int count = 0;
  int i = 0;
  int rc;
  int file_select ();
  char entry[MAXPATHLEN];
  struct dirent **entries;
  struct stat statbuf;


  //
  // Check if pathname is file or directory.
  //
  if ((rc = stat(pathname, &statbuf)) == -1) {
    printf("FAIL: %s. Could not obtain file status\n", pathname);
    goto EXIT;
  }

  //
  // If not directory, just check file permissions.
  //
  if (!(statbuf.st_mode & S_IFDIR)) {
    if (statbuf.st_uid != 0) {
      printf ("FAIL: %s. Invalid owner\n", entry);
      rc = -1;
      goto EXIT;
    }
    if ((statbuf.st_mode & S_IWGRP) || (statbuf.st_mode & S_IWOTH)) {
      printf ("FAIL: %s. Invalid write access\n", entry);
      rc = -1;
      goto EXIT;
    }
    printf ("PASS: %s\n", pathname);
    goto EXIT;
  }

  //
  // If directory, recurse through all subdirectories, checking all files.
  //
  if ((count = scandir (pathname, &entries, file_select, alphasort)) == -1) {
    rc = count;
    goto EXIT;
  }
  for (i = 0; i < count; i++) {
    sprintf(entry, "%s/%s", pathname, entries[i]->d_name);
    if (entries[i]->d_type == DT_DIR) {
      if ((rc = check_access (entry)) == -1) {
	goto EXIT;
      }
    } else {
      // ignore symlinks
      rc = lstat(entry, &statbuf);
      if(rc == 0 && S_ISLNK(statbuf.st_mode) == 1) {
    printf("INFO: %s/%s. Skipping symlink\n", pathname, entries[i]->d_name);
    continue;
      }
      if ((rc = stat(entry, &statbuf)) == -1) {
	printf("FAIL: %s. Could not obtain file status\n", entry);
	g_rc = -1;
	continue;
      }
      if (statbuf.st_uid != 0) {
	printf ("FAIL: %s. Invalid owner\n", entry);
	g_rc = -1;
	continue;
      }
      if ((statbuf.st_mode & S_IWGRP) || (statbuf.st_mode & S_IWOTH)) {
	printf ("FAIL: %s. Invalid write access\n", entry);
	g_rc = -1;
	continue;
      }

      printf ("PASS: %s\n", entry);
    }
  }

EXIT:

  return rc;
}

int main (int argc, char *argv[])
{
  int rc;

  if (argc != 2) {
    printf("Please enter target file or directory");
    rc = -1;
    goto EXIT;
  }

  rc = check_access (argv[1]);
  return rc;

 EXIT:
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
