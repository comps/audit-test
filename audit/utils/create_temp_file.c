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
**  FILE   : create_temp_file.c
**
**  PURPOSE: This file defines a utility function that creates a file.
**           Some test cases must operate on an existing file.  It is 
**           this function that guarantees that such files exist on the
**           system before the system call is issued by creating the
**           file at runtime.  This function uses mkstemp() to create
**           a temporary file at runtime.
**
**
**  HISTORY:
**    06/03 originated by Dustin Kirkland (k1rkland@us.ibm.com)
**
**********************************************************************/

#include "includes.h"
#include "tempname.h"

/*
** Create a test file
*/
int createTempFile(char **fname, mode_t mode, uid_t uid, gid_t gid)
{

    int rc = 0;
    int fd = 0;

    char *writedata = "This is bogus test file content\n";
    *fname = (char *)malloc(strlen(tempname) + 1);
    strcpy(*fname, tempname);

    printf5("createTempFile: %s\n", *fname);

    if ((fd = mkstemp(*fname)) == -1) {
	printf1("ERROR: Unable to create %s: errno=%i\n", *fname, errno);
	rc = fd;
	goto EXIT;
    }
    printf5("temp file name: %s\n", *fname);
    if ((rc = write(fd, writedata, strlen(writedata))) == -1) {
	printf1("ERROR: Unable to write data to file %s: errno=%i\n", *fname,
		errno);
	goto EXIT;
    }
    if ((rc = close(fd)) == -1) {
	printf1("ERROR: Unable to close file %s: errno=%i\n", *fname, errno);
	goto EXIT;
    }
    if ((rc = chmod(*fname, mode)) == -1) {
	printf1("ERROR: Unable to chmod file %s: errno=%i\n", *fname, errno);
	goto EXIT;
    }
    if ((rc = chown(*fname, uid, gid)) == -1) {
	printf1("ERROR: Unable to chown file %s: errno=%i\n", *fname, errno);
	goto EXIT;
    }

EXIT:
    return rc;

}
