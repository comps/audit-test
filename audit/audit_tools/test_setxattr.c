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
 **  FILE       : test_setxattr.c
 **
 **  PURPOSE    : To help test the aucat tool.
 **
 **  HISTORY    :
 **    08/03 Originated by Michael A. Halcrow <mike@halcrow.us>
 **
 **********************************************************************/

#ifndef NOXATTR

#include "includes.h"
#include "syscalls.h"
#include <attr/xattr.h>
#include <pwd.h>

#define XATTR_TEST_VALUE "text/plain"

int debug = 2;

int aucat_test_setxattr( int succeed, char* username );

int main( int argc, char* argv[] ) {

    aucat_test_setxattr( 1, argv[1] );
    aucat_test_setxattr( 0, argv[1] );
    return 0;
}
 
int aucat_test_setxattr( int succeed, char* username ) {
   
  int rc = 0;
  char* path = NULL;
  struct passwd *pw = NULL; 

  char* name = "user.mime_type";
  char value[ sizeof( XATTR_TEST_VALUE ) ];
  size_t size;
  int flags;

  if( succeed ) {
    printf( "Running success case...\n" );
  } else {
    printf( "Running error case...\n" );
    if (!(pw = getpwnam(username))) { 
    	goto EXIT;
    }
    
  }
 
  bzero( value, sizeof( XATTR_TEST_VALUE ) );
  size = sizeof( XATTR_TEST_VALUE );

    // Create the target file
    path = init_tempfile(S_IRWXU, 0, 0);
    if (!path) {
	rc = -1;
	goto EXIT;
    }

  //if( succeed ) {     // Set up for success
    flags = XATTR_CREATE;
    strcpy( value, XATTR_TEST_VALUE );
  //} else {        // Set up for error
  //  flags = XATTR_REPLACE;
  //  savePath = path;
  //  path = NULL;
  //}

 // for error case
 if (!succeed) { 
	if(seteuid(pw->pw_uid) == -1) {  //set euid to that of another user
             printf("Unable to set euid to %i, errno:%i", pw->pw_uid, errno);
             goto EXIT_CLEANUP;
        }
        if(setfsuid(pw->pw_uid) == -1) {
	     printf("Unable to set fsuid to %i, errno:%i", pw->pw_uid, errno);
	     goto EXIT_CLEANUP;
	}
  }
 
  syscall( __NR_setxattr, path, name, value, size, flags );

  //if( !succeed ) {
  //  path = savePath;
  //}

  if (!succeed) {
	if(seteuid(0) == -1)
           printf("Unable to set euid back to root errno=%i", errno);
        if(setfsuid(0) ==-1)
 	   printf("Unable to set fsuid back to root errno=%i", errno);
  } 	
 
EXIT_CLEANUP:
    destroy_temp(path);
 
EXIT:
    return rc;
}

#endif
