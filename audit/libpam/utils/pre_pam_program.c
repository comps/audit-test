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
**  FILE   : pre_pam_program.c
**
**  PURPOSE: This file defines a function that will call a series of 
**           functions that execute to prepare the test environment for
**           the pam program test.
**
**
**  HISTORY:
**    08/03 originated by Dustin Kirkland (k1rkland@us.ibm.com)
**
**********************************************************************/

#include "includes.h"
#include <time.h>

int prePAMProgram(struct audit_data* dataPtr) {

  int rc = 0;

  // su to test user
  printf( "setresgid to %i, %i, %i\n", dataPtr->gid, dataPtr->egid, 0 );
  if( ( rc = setresgid( dataPtr->gid, dataPtr->egid, 0 ) ) != 0 ) {
    printf( "Unable to setresgid to %i, %i, %i: errno=%i\n",
            dataPtr->gid, dataPtr->egid, 0, errno );
    goto EXIT;
  }
  printf( "setresuid to %i, %i, %i\n", dataPtr->uid, dataPtr->euid, 0 );
  if( ( rc = setresuid( dataPtr->uid, dataPtr->euid, 0 ) ) != 0 ) {
    printf( "Unable to setresuid to %i, %i, %i: errno=%i\n",
            dataPtr->uid, dataPtr->euid, 0, errno );
    goto EXIT;
  }

  // Fill in laus_data structure
  printf( "Calling getLAUSData\n" );
  if( ( rc = getLAUSData( dataPtr ) ) != 0 ) {
    printf( "Error returned from getLAUSData( dataPtr ): rc=%i\n", rc );
    goto EXIT;
  }

  //Forking a process will cause the saved ids to become that of the effective user
  //so a child cannot escalate its privledges
  dataPtr->suid = dataPtr->euid;
  dataPtr->sgid = dataPtr->egid;

  //The fs(u|g)id fields needed initializing
  dataPtr->fsgid = dataPtr->egid;
  dataPtr->fsuid = dataPtr->euid;

  printf( "Setting begin timestamp\n" );
#ifndef NOSLEEP
  sleep(2);
#endif
  dataPtr->begin_time = time( NULL );
  printf( "Performing PAM program test\n" );

 EXIT:
  return rc;

}
