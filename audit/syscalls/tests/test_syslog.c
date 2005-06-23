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
    **  FILE       : test_syslog.c
    **
    **  PURPOSE    : To test the syslog library call auditing.
    **
    **  DESCRIPTION: The test_syslog() function builds into the
    **  laus_test framework to verify that the Linux Audit System
    **  accurately logs both successful and erroneous execution of the
    **  "syslog" system call.
    **
    **  In the successful case, this function:
    **   1) Allocates memory for a buffer
    **   2) Executes the syslog() system call
    **
    **  The successful case correctly executes the syslog() system call
    **  by passing it a valid pointer to a memory buffer.
    **  
    **  In the erroneous case, this function:
    **   1) Does not allocate memory for a buffer
    **   2) Executes the syslog() system call with a NULL pointer for the
    **      buffer
    **      
    **  The erroneous case causes an EINVAL errno by passing a NULL pointer
    **  as the buffer pointer parameter to the syslog system call.
    **
    **  HISTORY    :
    **    06/03 Originated by Kylene J. Smith <kylene@us.ibm.com>
    **    11/03 Documentation by Dustin Kirkland <k1rkland@us.ibm.com>
    **    03/04 Added exp_errno variable by D. Kent Soper <dksoper@us.ibm.com>
    **    05/04 Updates to suppress compile warnings by Kimberly D. Simon <kdsimon@us.ibm.com>
    **
    **********************************************************************/
   
   #include "syscalls.h"
   #include <unistd.h>
   #include <linux/unistd.h>
   
   int test_syslog(laus_data* dataPtr) {
     
     int rc = 0;
     int exp_errno = EINVAL;
     char* buf = NULL;
     int len = 512;
     int type = 3;
     
     // Set the syscall-specific data
     printf5( "Setting laus_var_data.syscallData.code to %d\n", AUDIT_syslog );
     dataPtr->laus_var_data.syscallData.code = AUDIT_syslog;
     
     // Do as much setup work as possible right here

     if( dataPtr->successCase ) { // Set up for success
         if ( ( buf = (char*)malloc(len) ) == NULL ) {
   	  printf1("Could not allocate memory for buffer\n");
   	  goto EXIT;
         }
     } else { 

     }
   
     // Set up audit argument buffer
     if(( rc = auditArg3( dataPtr, 
			 AUDIT_ARG_IMMEDIATE, sizeof( int ), &type,
			 (dataPtr->successCase ? AUDIT_ARG_POINTER : AUDIT_ARG_NULL),
			 0, buf,
			 AUDIT_ARG_IMMEDIATE, sizeof( int ), &len)  != 0 )) {
       printf1( "Error setting up audit argument buffer\n" );
       goto EXIT_CLEANUP;
     }
   
     // Do pre-system call work
     if ( (rc = preSysCall( dataPtr )) != 0 ) {
       printf1("ERROR: pre-syscall setup failed (%d)\n", rc);
       goto EXIT_CLEANUP;
     }
   
     // Execute system call
     //dataPtr->laus_var_data.syscallData.result = syslog( type, buf, len );
     dataPtr->laus_var_data.syscallData.result = syscall( __NR_syslog, type, buf, len);
 
     // Do post-system call work
     if ( (rc = postSysCall(  dataPtr, errno, -1, exp_errno )) != 0 ) {
       printf1("ERROR: post-syscall setup failed (%d)\n", rc);
       goto EXIT_CLEANUP;
     }
   
    EXIT_CLEANUP:
     /**
      * Do cleanup work here
      */
     if( dataPtr->successCase && buf) {    // Clean up from success case setup
         free(buf);
     }
   
    EXIT:
     printf5( "Returning from test\n" );
     return rc;
   }
