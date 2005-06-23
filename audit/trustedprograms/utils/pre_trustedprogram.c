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
**  FILE   : pre_trustedprogram.c
**
**  PURPOSE: This file defines a function that will call a series of 
**           functions that execute to prepare the test environment for
**           the trusted program test.
**
**
**  HISTORY:
**    06/03 originated by Daniel H. Jones (danjones@us.ibm.com)
**
**********************************************************************/

#include "utils.h"
#include <time.h>

int preTrustedProgram(laus_data* dataPtr) {

  int rc = 0;

  // su to test user
  printf5( "setresgid to %i, %i, %i\n", dataPtr->msg_rgid, dataPtr->msg_egid, 0 );
  if( ( rc = setresgid( dataPtr->msg_rgid, dataPtr->msg_egid, 0 ) ) != 0 ) {
    printf1( "Unable to setresgid to %i, %i, %i: errno=%i\n",
            dataPtr->msg_rgid, dataPtr->msg_egid, 0, errno );
    goto EXIT;
  }
  printf5( "setresuid to %i, %i, %i\n", dataPtr->msg_ruid, dataPtr->msg_euid, 0 );
  if( ( rc = setresuid( dataPtr->msg_ruid, dataPtr->msg_euid, 0 ) ) != 0 ) {
    printf1( "Unable to setresuid to %i, %i, %i: errno=%i\n",
            dataPtr->msg_ruid, dataPtr->msg_euid, 0, errno );
    goto EXIT;
  }


  // Fill in laus_data structure
  printf5( "Calling getLAUSData\n" );
  if( ( rc = getLAUSData( dataPtr ) ) != 0 ) {
    printf1( "Error returned from getLAUSData( dataPtr ): rc=%i\n", rc );
    goto EXIT;
  }

  //Forking a process will cause the saved ids to become that of the effective user
  //so a child cannot escalate its privledges
  dataPtr->msg_suid = dataPtr->msg_euid;
  dataPtr->msg_sgid = dataPtr->msg_egid;

  //The fs(u|g)id fields needed initializing
  dataPtr->msg_fsgid = dataPtr->msg_egid;
  dataPtr->msg_fsuid = dataPtr->msg_euid;

  printf5( "Setting begin timestamp\n" );
#ifndef NOSLEEP
//  sleep(2);
#endif
  dataPtr->begin_r_time = time( NULL )-2;
  printf5( "Performing trusted program test\n" );

 EXIT:
  return rc;

}
