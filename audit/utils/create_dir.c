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
**  FILE   : create_dir.c
**
**  PURPOSE: This file defines a utility function that creates a directory.
**           Some test cases must operate on an existing directory.  It is 
**           this function that guarantees that the directory exists on the
**           system before the system call is issued by creating the
**           directory at runtime.
**
**
**  HISTORY:
**    05/03 originated by Dan Jones (danjones@us.ibm.com)
**    06/03 furthered by Dustin Kirkland (k1rkland@us.ibm.com
**    06/03 Converted to create_dir by Michael Halcrow <mhalcrow@cs.byu.edu>
**
**********************************************************************/

#include "includes.h"

/*
** Create a test directory
*/
int createDir(char *dname, mode_t mode, uid_t uid, gid_t gid)
{

    int rc = 0;

    printf5("createDir %s\n", dname);

    if ((rc = mkdir(dname, mode)) == -1) {
	printf1("ERROR: Unable to create %s: errno=%i\n", dname, errno);
	goto EXIT;
    }
    if ((rc = chown(dname, uid, gid)) == -1) {
	printf1("ERROR: Unable to chown directory %s: errno=%i\n", dname,
		errno);
	goto EXIT;
    }

EXIT:
    return rc;

}
