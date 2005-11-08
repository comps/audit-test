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
**  FILE   : create_temp_user_name.c
**
**  PURPOSE: This function attempts (as best as it can), to generate
**           a unique username, user home directory, and uid.  It will
**           allocated the memory necessary to store the user name
**           and the home directory strings.  It returns 0 on success.
**
**
**  HISTORY:
**    06/03 originated by Dustin Kirkland (k1rkland@us.ibm.com)
**    05/04 Updates to suppress compile warnings bu Kimberly D. Simon <kdsimon@us.ibm.com>
**
**********************************************************************/

#include "includes.h"
#include <pwd.h>

/*
** Generate unique user name, uid, homedir
*/
int createTempUserName(char **user, int *uid, char **homedir)
{

    int rc = 0;
    //int fd;   //not needed?
    char *base = "/home";
    char *usermask = "laus_";
    char c = 'a';


    // Malloc memory for data to be returned to the caller
    // NOTE: Caller must free() this memory!!!
    *homedir = (char *)malloc(strlen(base) + strlen(usermask) + 3);
    *user = (char *)malloc(strlen(usermask) + 2);

    do {
	// Construct home directory mask
	sprintf(*user, "%s%c", usermask, c);
	sprintf(*homedir, "%s/%s%c", base, usermask, c);
	c++;
    } while ((getpwnam(*user) != NULL) && (c <= 'z'));

    // Got username, and home directory, must get unique uid
    *uid = 700;

    // Look for a unique uid, such that 700<uid<1000
    do {
	(*uid)++;
    } while ((getpwuid(*uid) != NULL) && (*uid < 1000));

    if ((*uid >= 1000) || (c > 'z')) {
	rc = -1;
	*user = NULL;
	*uid = -1;
	*homedir = NULL;
    }

    //EXIT:  // not needed?
    return rc;

}
