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
**  FILE   : post_pam_program.c
**
**  PURPOSE: This file defines a function that will call a series of 
**           functions that execute to after the pam program.
**
**
**  HISTORY:
**    06/03 originated by Daniel H. Jones (danjones@us.ibm.com)
**    08/03 modified by Dustin Kirkland (k1rkland@us.ibm.com)
**
**********************************************************************/

#include "includes.h"
#include <time.h>

int postPAMProgram(laus_data* dataPtr) {

  int rc = 0;

  printf5( "Setting end timestamp\n" );
#ifndef NOSLEEP
  sleep(2);
#endif
  dataPtr->end_r_time = time( NULL );

  // su back to root
  printf5( "setresuid to root\n" );
  if ( ( rc = setresuid( 0, 0, 0 ) ) != 0 ) {
    printf1( "Unable to setresuid to root: errno=%i\n", errno );
    goto EXIT;
  }

  printf5( "setresgid to root\n" );
  if ( ( rc = setresgid( 0, 0, 0 ) ) != 0 ) {
    printf1( "Unable to setresgid to root: errno=%i\n", errno );
    goto EXIT;
  }


 EXIT:
  return rc;

}
