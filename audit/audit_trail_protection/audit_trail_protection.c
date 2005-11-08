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
**  FILE   : audit_tail_protection.c
**
**  PURPOSE: Verify that the file permissions for the audit log files
**  are set to be readable and writable only by the superuser.
**
**
**  HISTORY:
**    09/03 Originated by Michael A. Halcrow <mike@halcrow.us>
**
**********************************************************************/

#include "includes.h"
#include <regex.h>

int debug = 2;

int testFilePermissions( char* filename ) {

  int rc = 0;
  struct stat buf;

  printf4( "Testing file permissions: [%s]\n", filename );

  if(( rc = stat( filename, &buf ))) {
    printf1( "Error attempting to stat file [%s]\n", filename );
    return rc;
  }
  
  if( buf.st_uid != 0 ) {
    printf2( "Audit log permissions test failed on [%s]: owner not superuser\n", filename );
    rc = 1;
    goto EXIT;
  }
  
  if( buf.st_mode & 0x0007F ) {
    printf2( "Audit log permissions test failed on [%s]: permissions insecure\n", filename );
    rc = 1;
    goto EXIT;
  }  

 EXIT:
  return rc;

}

int main() {
  int rc = 0;
  FILE* f;
  char data[ 4096 ];
  char basename[ 256 ];
  char filename[ 260 ];
  //int x, y;
  int x;	
  regex_t preg;
  const char* regex = "file-name[ \t]*=[ \t]*\"\\([^\"]*\\)\";";
  int cflags = 0;
  regmatch_t pmatch[ 256 ];
  int eflags = 0;
  int passed = 0, failed = 0, skipped = 0;

  if( (rc = testFilePermissions( "/etc/audit/audit.conf" )) ) {
    failed++;
  } else {
    passed++;
  }

  if( (rc = testFilePermissions( "/etc/audit/filesets.conf" )) ) {
    failed++;
  } else {
    passed++;
  }

  if( (rc = testFilePermissions( "/etc/audit/filter.conf" )) ) {
    failed++;
  } else {
    passed++;
  }

  if( (rc = regcomp( &preg, regex, cflags )) ) {
    printf1( "Error compiling regex: [%s]\n", regex );
    skipped += 4;
    goto EXIT;
  }

  if( !( f = fopen( "/etc/audit/audit.conf", "r" ) ) ) {
    printf1( "Error opening /etc/audit/audit.conf for reading\n" );
    rc = 1;
    skipped += 4;
    goto EXIT;
  }

  if( !( rc = fread( data, 1, 4096, f ) ) ) {
    printf( "Error reading /etc/audit/audit.conf\n" );
    rc = 1;
    skipped += 4;
    goto EXIT;
  }

  data[ rc ] = 0;

  fclose( f );

  if( ( rc = regexec( &preg, data, 2, pmatch, eflags ) ) ) {
    printf( "No match found\n" );
    skipped += 4;
    goto EXIT;
  }

  if( ( pmatch[1].rm_eo - pmatch[1].rm_so ) >= 256 ) {
    printf( "Audit path longer than 255 characters\n" );
    rc = 1;
    skipped += 4;
    goto EXIT;
  }

  strncpy( basename, &data[ pmatch[1].rm_so ], pmatch[1].rm_eo - pmatch[1].rm_so );

  basename[ pmatch[1].rm_eo - pmatch[1].rm_so ] = '\0';

  for( x = 0; x < 3; x++ ) {

    sprintf( filename, "%s.%d", basename, x );

    if(( rc = testFilePermissions( filename ))) {
      failed++;
    } else {
      passed++;
    }

  }

 EXIT:

  printf2( "PASSED = %d, FAILED = %d, SKIPPED = %d\n", passed, failed, skipped );

  return rc;
}
