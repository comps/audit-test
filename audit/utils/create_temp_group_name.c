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
 **  FILE   : create_temp_group_name.c
 **
 **  PURPOSE: This function attempts (as best as it can), to generate
 **           a unique username, user home directory, and uid.  It will
 **           allocated the memory necessary to store the user name
 **           and the home directory strings.  It returns 0 on success.
 **
 **
 **  HISTORY:
 **    06/03 createTempUserName originated by Dustin Kirkland
 **          (k1rkland@us.ibm.com)
 **    07/03 createTempGroupName adapted by Michael A. Halcrow
 **          <mike@halcrow.us>
 **    05/04 Updates to suppress compile warnings by Kimberly D. Simon <kdsimon@us.ibm.com>
 **
 **********************************************************************/

#include "includes.h"
#include <grp.h>

/*
** Generate unique group name and gid
*/
int createTempGroupName(char** group, int* gid ) {

  int rc = 0;
  //int fd;         // not needed
  char* groupmask = "lausg_";
  char c = 'a';

  // Malloc memory for data to be returned to the caller
  // NOTE: Caller must free() this memory
  *group = (char*)malloc( strlen( groupmask ) + 2 );

  // Construct group name from mask
  do {
    sprintf( *group, "%s%c", groupmask, c );
    c++;
  } while( ( getgrnam( *group ) != NULL) && ( c <= 'z' ) );

  // Got groupname; must get unique gid
  (*gid) = 600;

  // Look for a unique gid, such that 600<gid<1000
  while( ( getgrgid( *gid ) != NULL ) && ( (*gid) < 1000 ) ) {
    (*gid)++;
  }

  if( ( (*gid) >= 1000 ) || ( c >= 'z' ) ) {
    rc = -1;
    (*group) = NULL;
    goto EXIT;
  }

 EXIT:
  return rc;

}

