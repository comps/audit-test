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
**  FILE   : create_temp_dir.c
**
**  PURPOSE: This file defines a utility function that creates a dir.
**           Some test cases must operate on an existing dir.  It is 
**           this function that guarantees that such dirs exist on the
**           system at runtime.
**
**
**  HISTORY:
**    06/03 originated by Dustin Kirkland (k1rkland@us.ibm.com)
**
**********************************************************************/

#include "includes.h"
#include "tempname.h"

/*
** Create a temporary directory
*/
int createTempDir(char **dname, mode_t mode, uid_t uid, gid_t gid)
{

    int rc = -1;
    char *dirname;
    *dname = (char *)malloc(strlen(tempname) + 1);
    dirname = (char *)malloc(strlen(tempname) + 1);
    strcpy(*dname, tempname);

    fprintf(stderr, "createTempDir: %s\n", *dname);

    if ((dirname = mkdtemp(*dname)) == NULL) {
	fprintf(stderr, "Error: unable to create %s: errno=%i\n", 
		*dname, errno);
	goto EXIT;
    }
    rc = 0;
    fprintf(stderr, "temp dir name: %s\n", *dname);
    if ((rc = chmod(*dname, mode)) == -1) {
	fprintf(stderr, "Error: unable to chmod dir %s: errno=%i\n", 
		*dname, errno);
	goto EXIT;
    }
    if ((rc = chown(*dname, uid, gid)) == -1) {
	fprintf(stderr, "Error: unable to chown dir %s: errno=%i\n", 
		*dname, errno);
	goto EXIT;
    }

EXIT:
    return rc;

}
