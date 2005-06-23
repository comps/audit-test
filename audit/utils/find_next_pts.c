/**********************************************************************
**   Copyright (C) 2003 International Business Machines Corp.
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
**  FILE   : find_next_pts.c
**
**  PURPOSE: To determine the next pts value that will be assigned
**  upon login.
**
**  HISTORY:
**    10/03 Originated by Michael A. Halcrow <mike@halcrow.us>
**
**********************************************************************/
#include "../include/includes.h"

#define MAX_PTS_CHARACTERS 10

int find_next_pts() {
  FILE* fPtr;
  char* ptsData;
  int pts = 0;
  system( "/usr/bin/perl -e '@files = `ls /dev/pts`; $free_pts = 0; for( $x = 0; $x < 42; $x += 1 ) { foreach $file ( <@files> ) { if( \"$x\" eq $file ) { goto NEXT; } } $free_pts = \"$x\"; goto FOUND; NEXT: } FOUND: print $free_pts.\"\n\";' > /tmp/nextPts.txt" );
  if( ( fPtr = fopen( "/tmp/nextPts.txt", "r" ) ) == NULL ) {
    printf1( "Error opening pts file: [%s]\n", "/tmp/nextPts.txt" );
    return -1;
  }
  ptsData = (char*)malloc( MAX_PTS_CHARACTERS );
  if( fread( (void*)ptsData, 1, ( MAX_PTS_CHARACTERS-1 ), fPtr ) == 0 ) {
    printf1( "Error reading pts value\n" );
    return -1;
  }
  fclose( fPtr );
  if( unlink( "/tmp/nextPts.txt" ) != 0 ) {
    printf1( "Error unlinking /tmp/nextPts.txt\n" );
    return -1;
  }
  ptsData[ ( MAX_PTS_CHARACTERS-1 ) ] = '\0';
  if( ( strlen( ptsData ) > 0 ) && ( ptsData[ strlen( ptsData )-1 ] == '\n' ) ) {
    ptsData[ strlen( ptsData )-1 ] = '\0';
  }
  pts = atoi( ptsData );
  free( ptsData );
  return pts;
}
