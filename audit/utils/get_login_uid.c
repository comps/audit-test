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
**  FILE   : get_login_uid.c
**
**  PURPOSE: This file defines a function that gets the uid of the user
**           that actually logged into the terminal session which is 
**           running the test suite.
**
**
**  HISTORY:
**    09/03 originated by Dustin Kirkland (k1rkland@us.ibm.com)
**    10/03 Modified by Michael A. Halcrow <mike@halcrow.us>
**
**********************************************************************/

#include "includes.h"

int getLoginUID()
{
    int rc = -1;
    FILE *fPtr;
    char *luid_filename;
    char *command;

    createTempFileName(&luid_filename);
//  command = mysprintf( "/usr/bin/who -m | /usr/bin/awk -F! '{print $2}' | /usr/bin/awk -F\" \" '{print $1}' | /usr/bin/xargs -i id -u {} > %s", luid_filename); 
    command =
	mysprintf("who -m | awk -F\" \" '{print $1}' | xargs id -u > %s",
		  luid_filename);
    if ((rc = system(command)) == -1) {
	printf1("ERROR: Get login uid failed\n");
	goto EXIT;
    }
    // Get the pts used
    if ((fPtr = fopen(luid_filename, "r")) == NULL) {
	printf1("Error opening file [%s] for read access\n", luid_filename);
	goto EXIT;
    }
    fscanf(fPtr, "%d", &rc);
    fclose(fPtr);

  EXIT:
    unlink(luid_filename);
    free(luid_filename);
    free(command);

    return rc;
}
