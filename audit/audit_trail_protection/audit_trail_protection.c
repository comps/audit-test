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

  printf( "Testing file permissions: [%s]\n", filename );

  if(( rc = stat( filename, &buf ))) {
    printf( "Error attempting to stat file [%s]\n", filename );
    return rc;
  }
  
  if( buf.st_uid != 0 ) {
    printf( "Audit log permissions test failed on [%s]: owner not superuser\n", filename );
    rc = 1;
    goto EXIT;
  }
  
  if( buf.st_mode & 0x0004F ) {
    printf( "Audit log permissions test failed on [%s]: permissions insecure\n", filename );
    rc = 1;
    goto EXIT;
  }  

 EXIT:
  return rc;

}

int main() {
  int rc = 0;
  int passed = 0, failed = 0, skipped = 0;

  if( (rc = testFilePermissions( "/etc/auditd.conf" )) ) {
    failed++;
  } else {
    passed++;
  }

  if( (rc = testFilePermissions( "/etc/audit.rules" )) ) {
    failed++;
  } else {
    passed++;
  }

  printf( "PASSED = %d, FAILED = %d, SKIPPED = %d\n", passed, failed, skipped );

  return rc;
}
