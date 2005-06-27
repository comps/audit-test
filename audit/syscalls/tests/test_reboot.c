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
 **  FILE       : test_reboot.c
 **
 **  PURPOSE    : To test the reboot library call auditing.
 **
 **  DESCRIPTION: The test_reboot() function builds into the
 **  laus_test framework to verify that the Linux Audit System
 **  accurately logs both successful and erroneous execution of the
 **  "reboot" system call.
 **
 **  In the successful case, this function:
 **   1) Sets the euid to the superuser
 **   2) Calls reboot with flag=RB_ENABLE_CAD
 **   3) Sets the euid to the test user
 **   4) Verifies the success result.
 **
 **  The successful case enables the Ctrl-Alt-Del keystroke.  Since
 **  reboot is called by the superuser, it will succeed, according to
 **  the man page.
 **  
 **  In the erroneous case, this function:
 **   1) Sets the euid to the test user
 **   2) Calls reboot with flag=RB_ENABLE_CAD
 **   3) Sets the euid to the superuser
 **   4) Verifies the error result.
 **      
 **  The erroneous case calls reboot as a user other than the
 **  superuser, which, according to the man page, is an error
 **  condition.
 **
 **  HISTORY    :
 **    06/03 Originated by Michael A. Halcrow <mike@halcrow.us>
 **    03/04 Added exp_errno variable by D. Kent Soper <dksoper@us.ibm.com>
 **
 **********************************************************************/
   
#include "includes.h"
#include "syscalls.h"
#include <linux/reboot.h>
   
int test_reboot(laus_data* dataPtr) {
     
  int rc = 0;
  int exp_errno = EPERM; 
  //     int flag = RB_ENABLE_CAD;
  int flag = LINUX_REBOOT_CMD_CAD_ON;
  int magic1 = LINUX_REBOOT_MAGIC1;
  int magic2 = LINUX_REBOOT_MAGIC2;
  void* arg = NULL;
     
  // Set the syscall-specific data
  printf5( "Setting laus_var_data.syscallData.code to %d\n", AUDIT_reboot );
  dataPtr->laus_var_data.syscallData.code = AUDIT_reboot;
     
  /**
   * Do as much setup work as possible right here
   */
  if( dataPtr->successCase ) {
    // Set up for success
    dataPtr->msg_euid = 0;
    dataPtr->msg_egid = 0;
    dataPtr->msg_fsuid = 0;
    dataPtr->msg_fsgid = 0;
    // TODO: Save the current flag value?
  } else {
    // Set up for error
  }
   
  // Set up audit argument buffer
  if( ( rc = auditArg4( dataPtr,
			AUDIT_ARG_IMMEDIATE, sizeof( int ), &magic1,
			AUDIT_ARG_IMMEDIATE, sizeof( int ), &magic2,
			AUDIT_ARG_IMMEDIATE_u, sizeof( int ), &flag,
			AUDIT_ARG_NULL, 0, arg ) ) != 0 ) {
    printf1( "Error setting up audit argument buffer\n" );
    goto EXIT;
  }
   
  // Do pre-system call work
  if ( (rc = preSysCall( dataPtr )) != 0 ) {
    printf1("ERROR: pre-syscall setup failed (%d)\n", rc);
    goto EXIT_CLEANUP;
  }
   
  // Execute system call
  //     dataPtr->laus_var_data.syscallData.result = syscall( __NR_reboot, flag );
  dataPtr->laus_var_data.syscallData.result = syscall( __NR_reboot, magic1, magic2, flag, arg );

   
  // Do post-system call work
  if ( (rc = postSysCall(  dataPtr, errno, -1, exp_errno  )) != 0 ) {
    printf1("ERROR: post-syscall setup failed (%d)\n", rc);
    goto EXIT_CLEANUP;
  }
   
 EXIT_CLEANUP:
  /**
   * Do cleanup work here
   */
  if( dataPtr->successCase ) {
    // Clean up from success case setup
  }
   
 EXIT:
  printf5( "Returning from test\n" );
  return rc;
}
