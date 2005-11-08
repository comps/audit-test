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
 **  FILE       : test_brk.c
 **
 **  PURPOSE    : To test the brk library call auditing.
 **
 **  DESCRIPTION: The test_brk() function builds into the
 **  laus_test framework to verify that the Linux Audit System
 **  accurately logs both successful and erroneous execution of the
 **  "brk" system call.
 **
 **  In the successful case, this function:
 **   1) Discovers the current location of the program break
 **   2) Makes the brk syscall with that value.
 **
 **  The successful case guarantees success because it does not
 **  attempt to allocate any new resources.  This excludes the
 **  possibility of invoking the only error condition defined in the
 **  man page for brk, ENOMEM.  We can therefore expect a success
 **  result.
 **  
 **  The erroneous case is not tested, due to the fact that forcing a
 **  memory allocation error could invalidate the test environment.
 **
 **  HISTORY    :
 **    09/03 Originated by Michael A. Halcrow <mike@halcrow.us
 **    03/04 Added exp_errno variable by D. Kent Soper <dksoper@us.ibm.com>
 **
 **********************************************************************/

#include "syscalls.h"
#include <sys/types.h>
#include <sys/wait.h>

int test_brk(laus_data* dataPtr) {
  
  int rc = 0;
  int exp_errno = ENOMEM;
  int pid;
  int status = 0;
  
  // For some stack magic in the forked child
  int* resultPtr = &(dataPtr->laus_var_data.syscallData.result);
  void* end_data_segment;
  
  // Set the syscall-specific data
  printf5( "Setting laus_var_data.syscallData.code to %d\n", AUDIT_brk );
  dataPtr->laus_var_data.syscallData.code = AUDIT_brk;
  //Do as much setup work as possible right here

  if ( ! dataPtr->successCase ) {
      rc = SKIP_TEST_CASE;
      goto EXIT;
  }

  end_data_segment = sbrk(0);  

  // Set up audit argument buffer
  if( ( rc = auditArg1( dataPtr,
			AUDIT_ARG_POINTER, 0, end_data_segment ) ) != 0 ) {
      printf1( "Error setting up audit argument buffer\n" );
      goto EXIT;
  }
  
  // Do pre-system call work
  preSysCall( dataPtr );
  
  // Execute system call
  dataPtr->laus_var_data.syscallData.result = syscall(__NR_brk, end_data_segment);

  // Do post-system call work
  postSysCall( dataPtr, errno, -1, exp_errno );
  
 EXIT_CLEANUP:
  // Do cleanup work here
  
 EXIT:
  printf5( "Returning from test\n" );
  return rc;
}
