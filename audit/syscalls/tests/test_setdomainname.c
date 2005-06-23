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
   **  FILE       : test_setdomainname.c
   **
   **  PURPOSE    : To test the setdomainname library call auditing.
   **
   **  DESCRIPTION: The test_setdomainname() function builds into the
   **  laus_test framework to verify that the Linux Audit System
   **  accurately logs both successful and erroneous execution of the
   **  "setdomainname" system call.
   **
   **  In the successful case, this function:
   **   1) Gets the current domainname
   **   2) Sets the euid to the superuser
   **   3) Calls setdomainname with the current domainname
   **   4) Verifies the success result.
   **
   **  The successful case passes a valid domainname and length to the
   **  setdomainname system call as the superuser.  According to the man
   **  page for setdomainname, this will result in a success.
   **  
   **  In the erroneous case, this function:
   **   1) Sets the euid to the test user
   **   2) Calls setdomainname with a dummy domainname
   **   3) Sets the euid to the superuser
   **   4) Verifies the error result.
   **      
   **  The erroneous case calls setdomainname as the test user.  According
   **  to the man page for setdomainname, if the caller was not the
   **  superuser, then an EPERM error will result.
   **
   **  HISTORY    :
   **    06/03 Originated by Michael A. Halcrow <mike@halcrow.us>
   **    03/04 Added exp_errno variable by D. Kent Soper <dksoper@us.ibm.com>
   **    05/04 Updates to suppress compile warnings by Kimberly D. Simon <kdsimon@us.ibm.com>
   **
   **********************************************************************/
  
  #include "syscalls.h"
  #include <linux/utsname.h>
  
  int test_setdomainname(laus_data* dataPtr) {
    
    int rc = 0;
    int exp_errno = EPERM;
  
    char name[ __NEW_UTS_LEN ];
    size_t len = __NEW_UTS_LEN;
    
    // Set the syscall-specific data
    printf5( "Setting laus_var_data.syscallData.code to %d\n", AUDIT_setdomainname );
    dataPtr->laus_var_data.syscallData.code = AUDIT_setdomainname;

    if( getdomainname( name, len ) == -1 ) {
        printf1( "Cannot get current domainname\n" );
        goto EXIT;
    }
    printf4( "Current domainname = [%s]\n", name );
    len = strlen( name );
    
    // Do as much setup work as possible right here
    if( dataPtr->successCase ) {
	// Set up for success
	dataPtr->msg_euid = 0;
	dataPtr->msg_egid = 0;
	dataPtr->msg_fsuid = 0;
	dataPtr->msg_fsgid = 0;
	// Get the current domainname
    } else {
      // Set up for error
    }
  
    // Set up audit argument buffer
    if( ( rc = auditArg2( dataPtr,
                          dataPtr->successCase ? AUDIT_ARG_POINTER : AUDIT_ARG_NULL,
                          dataPtr->successCase ? strlen( name ) : 0,
                          dataPtr->successCase ? &name : NULL,
                          AUDIT_ARG_IMMEDIATE, sizeof( size_t ), &len ) ) != 0 ) {
      printf1( "Error setting up audit argument buffer\n" );
      goto EXIT;
    }
  
    // Do pre-system call work
    preSysCall( dataPtr );
  
    // Execute system call
    dataPtr->laus_var_data.syscallData.result = syscall( __NR_setdomainname, name, len );
  
    // Do post-system call work
    postSysCall( dataPtr, errno, -1, exp_errno );
  
   //EXIT_CLEANUP:
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
