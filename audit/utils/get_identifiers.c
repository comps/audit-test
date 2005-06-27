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
 **  FILE       : get_identifiers.c
 **
 **  PURPOSE    : To determine the R*ID, E*ID, S*ID, and FS*ID of the
 **  current process.  We need to know these values for the set*id()
 **  call tests.
 **
 **
 **  HISTORY    :
 **    06/03 Originated by Michael A. Halcrow <mike@halcrow.us>
 **    05/04 Updates to suppress compile warnings by Kimberly D. Simon <kdsimon@us.ibm.com>
 **
 **********************************************************************/

#include "includes.h"

int getIdentifiers( identifiers_t* identifiers ) {
  int fd;
  char data[ 2048 ];
  if( ( fd = open( "/proc/self/status", O_RDONLY ) ) == -1 ) {
    printf( "Error opening '/proc/self/status' for read access\n" );
    return -1;
  }
  if( read( fd, data, 2048 ) ) {
    char* next;
    //char temp[10];
    close( fd );
    next = strtok( data, "\t" );
    if( strlen( next ) > 5 ) {
      next += ( strlen( next ) - 5 );
    }
    while( next && strncmp( next, "\nUid:", 5 ) ) {
      next = strtok( NULL, "\t" );
      if( strlen( next ) > 5 ) {
	next += ( strlen( next ) - 5 );
      }
    }
    if( !next ) {
      printf( "Error tokenizing /proc/self/status\n" );
      return -1;
    }
    next = strtok( NULL, "\t" );
    if( next )
      identifiers->ruid = atoi( next );  // TODO: Make sure these are in the right order
    next = strtok( NULL, "\t" );
    if( next )
      identifiers->euid = atoi( next );
    next = strtok( NULL, "\t" );
    if( next )
      identifiers->suid = atoi( next );
    next = strtok( NULL, "\n" );
    if( next )
      identifiers->fsuid = atoi( next );
    next = strtok( NULL, "\t" );
    next = strtok( NULL, "\t" );
    if( next )
      identifiers->rgid = atoi( next );
    next = strtok( NULL, "\t" );
    if( next )
      identifiers->egid = atoi( next );
    next = strtok( NULL, "\t" );
    if( next )
      identifiers->sgid = atoi( next );
    next = strtok( NULL, "\n" );
    if( next )
      identifiers->fsgid = atoi( next );
    return 0;
  } else {
    close( fd );
  }
  return 0;
}
