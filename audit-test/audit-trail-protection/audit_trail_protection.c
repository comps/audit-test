/*  Copyright (C) International Business Machines  Corp., 2003
 *  (c) Copyright Hewlett-Packard Development Company, L.P., 2005
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of version 2 the GNU General Public License as
 *  published by the Free Software Foundation.
 *  
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *  
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 *  FILE:
 *  audit_trail_protection.c
 *
 *  PURPOSE: Verify that the file permissions for the audit log files
 *  are set to be readable and writable only by the superuser.
 *
 *  HISTORY:
 *    09/03 Originated by Michael A. Halcrow <mike@halcrow.us>
 *    11/05 Updated for RHEL audit subsystem
 */

#include <stdio.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

int testFilePermissions( char* filename ) {
  struct stat buf;

  printf( "Testing file permissions: [%s]\n", filename );

  if(( stat( filename, &buf ))) {
    printf( "Error attempting to stat file [%s]\n", filename );
    return 1;
  }
  
  if( buf.st_uid != 0 ) {
    printf( "Audit log permissions test failed on [%s]: owner not superuser\n", filename );
    return 1;
  }
  
  if( buf.st_mode & 0x0004F ) {
    printf( "Audit log permissions test failed on [%s]: permissions insecure\n", filename );
    return 1;
  }  

  return 0;
}

int main() {
  int rc = 0;
  rc |= testFilePermissions( "/etc/audit/auditd.conf" );
  rc |= testFilePermissions( "/etc/audit/audit.rules" );
  return rc;
}
