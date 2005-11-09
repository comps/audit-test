/**********************************************************************
**   Copyright (C) International Business Machines  Corp., 2003
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
**  FILE   : file_utils.c
**
**  PURPOSE: This file contains general purpose file utility functions.
**
**  HISTORY:
**    07/03 originated by Daniel H. Jones (danjones@us.ibm.com)
**    06/04 Added AUDIT_ARCH_S390X by Kimberly D. Simon (kdsimon@us.ibm.com)
**
**********************************************************************/

#include "includes.h"

/*
** Create a backup file with a .bak extention
*/
int backupFile(char *fname)
{

    int rc = 0;
    int fd_file = 0;
    int fd_bak = 0;
    char *backup_fname = NULL;
    char *file_buffer = NULL;
    struct stat st;

    // Allocate space for backup file name
    backup_fname = (char *)malloc(strlen(fname) + 5);

    // Create backup file name
    if ((rc = sprintf(backup_fname, "%s.bak", fname)) == -1) {
	goto EXIT;
    }
    // Open source file
    if ((fd_file = open(fname, O_RDONLY)) == -1) {
	rc = -1;
	goto EXIT;
    }
    // Get source file mode
    if ((rc = fstat(fd_file, &st)) == -1) {
	goto EXIT;
    }
    // Create backup file with same mode
    if ((fd_bak =
	 open(backup_fname, O_CREAT | O_WRONLY | O_EXCL, st.st_mode)) == -1) {
	/* if backup file exists already, don't overwrite it. Return, treating as successful. */
	rc = (errno == EEXIST) ? 0 : -1;
	goto EXIT;
    }
    // Allocate file buffer
    file_buffer = (char *)malloc(st.st_size);

    // Read source file
    if ((rc = read(fd_file, file_buffer, st.st_size)) == -1) {
	goto EXIT;
    }
    // Write backup file
    if ((rc = write(fd_bak, file_buffer, st.st_size)) == -1) {
	goto EXIT;
    }

    printf5("Copy file %s to %s\n", fname, backup_fname);

EXIT:

    if (fd_file != 0) {
	close(fd_file);
    }

    if (fd_bak != 0) {
	close(fd_bak);
    }

    if (backup_fname != NULL) {
	free(backup_fname);
    }

    if (file_buffer != NULL) {
	free(file_buffer);
    }

    return rc;
}

/*
** Restore file from backup
*/
int restoreFile(char *fname)
{

    int rc = 0;
    char *backup_fname = NULL;

    // Allocate space for backup file name
    backup_fname = (char *)malloc(strlen(fname) + 5);

    // Create backup file name
    if ((rc = sprintf(backup_fname, "%s.bak", fname)) == -1) {
	goto EXIT;
    }
    // Just rename backup to original name
    rc = rename(backup_fname, fname);

    printf5("Restored file %s from %s\n", fname, backup_fname);

EXIT:

    if (backup_fname != NULL) {
	free(backup_fname);
    }

    return rc;
}
