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
**  FILE   : post_syscall.c
**
**  PURPOSE: This file defines a function that will call a series of 
**           functions that execute to after the system call.
**
**
**  HISTORY:
**    06/03 originated by Dustin Kirkland (k1rkland@us.ibm.com)
**                    and Michael Halcrow (mhalcrow@cs.byu.edu)
**    05/04 Updates to suppress compile warnings by Kimberly D. Simon <kdsimon@us.ibm.com>
**
**********************************************************************/

#include "../../include/utils.h"
#include <time.h>

int postSysCall(laus_data* dataPtr, int resultErrno, int errorRC, int expectedErrno) {
  int rc = 0;

  printf5( "Setting end timestamp\n" );
  dataPtr->end_r_time = time( NULL )+2; // MH: 2 seconds past current
					// time

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

  // Save resulting errno into data structure
  if (dataPtr->laus_var_data.syscallData.result == -1) {
	dataPtr->laus_var_data.syscallData.resultErrno = resultErrno;
  } else {
	dataPtr->laus_var_data.syscallData.resultErrno = 0;
  }

  // Check if the syscall executed as expected.
  printf5( "Checking to see if the system call executed as expected\n" );
  if( dataPtr->laus_var_data.syscallData.result == errorRC ) {
    if( dataPtr->successCase ) {
      printf2( "SYSCALL ERROR: %s unsuccessful in successful case:", dataPtr->testName );
      rc = 1;
    } else {
      if( resultErrno == expectedErrno ) {
        printf2( "SYSCALL SUCCESS: %s unsuccessful in unsuccessful case:", dataPtr->testName );
      } else {
        printf2( "SYSCALL ERROR: %s unsuccessful, but errno is different than expected (%i):", dataPtr->testName, expectedErrno );
        rc = 1;
      }
    }
  } else {
    if( dataPtr->successCase ) {
      printf2( "SYSCALL SUCCESS: %s successful in successful case:", dataPtr->testName );
    } else {
      printf2( "SYSCALL ERROR: %s successful in unsuccessful case:", dataPtr->testName);
      rc = 1;
    }
  }

  printf( " rc=%i, errno=%i\n", dataPtr->laus_var_data.syscallData.result, dataPtr->laus_var_data.syscallData.resultErrno );

 EXIT:
  return rc;

}
