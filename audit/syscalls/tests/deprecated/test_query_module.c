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
    **  FILE       : test_query_module.c
    **
    **  PURPOSE    : To test the query_module library call auditing.
    **
    **  DESCRIPTION: The test_query_module() function builds into the
    **  laus_test framework to verify that the Linux Audit System
    **  accurately logs both successful and erroneous execution of the
    **  "query_module" system call.
    **
    **  In the successful case, this function:
    **   1) Calls query_module with which=0
    **   2) Verifies the success result.
    **
    **  In the successful case, which=0.  According to the man page, when
    **  which=0, query_module always returns success.
    **  
    **  In the erroneous case, this function:
    **   1) Calls query_module with which=42
    **   2) Verifies the error result.
    **      
    **  The erroneous case causes query_module to return errno EINVAL due
    **  to the fact that 42 is an invalid value for which.
    **
    **  HISTORY    :
    **    06/03 Originated by Michael A. Halcrow <mike@halcrow.us>
    **    03/04 Added exp_errno variable by D. Kent Soper <dksoper@us.ibm.com>
    **
    **********************************************************************/
   
   #include "syscalls.h"
   #include <asm/atomic.h>

   struct module_info {
     unsigned long address;
     unsigned long size;
     unsigned long flags;
   };
   
   int test_query_module(laus_data* dataPtr) {
     
     int rc = 0;
     int exp_errno = EINVAL;
   
     char* name = NULL; // Indicating the kernel proper
     int which;
     struct module_info buf;
     size_t bufsize = sizeof( struct module_info );
     size_t ret;
     
     // Set the syscall-specific data
     printf5( "Setting laus_var_data.syscallData.code to %d\n", __NR_query_module );
     dataPtr->laus_var_data.syscallData.code = __NR_query_module;
     
     /**
      * Do as much setup work as possible right here
      */
     if( dataPtr->successCase ) {
       // Set up for success
       // Might include: dataPtr->msg_euid = 0; dataPtr->msg_egid = 0;
       which = 0; // Always returns success
     } else {
       // Set up for error
       which = 42;
     }
   
     // Set up audit argument buffer
     if( ( rc = auditArg5( dataPtr,
   		 AUDIT_ARG_NULL, 0, name,
   		 AUDIT_ARG_IMMEDIATE, sizeof( int ), &which,
   		 AUDIT_ARG_POINTER, 0, &buf,
   		 AUDIT_ARG_IMMEDIATE, sizeof( size_t ), &bufsize,
   		 AUDIT_ARG_POINTER, 0, &ret ) ) != 0 ) {
       printf1( "Error setting up audit argument buffer\n" );
       goto EXIT;
     }
   
     // Do pre-system call work
     if ( (rc = preSysCall( dataPtr )) != 0 ) {
       printf1("ERROR: pre-syscall setup failed (%d)\n", rc);
       goto EXIT;
     }
   
     // Execute system call
     dataPtr->laus_var_data.syscallData.result = syscall( __NR_query_module, name, which, (void*)&buf, bufsize, &ret );
   
     // Do post-system call work
     if ( (rc = postSysCall(  dataPtr, errno, -1, exp_errno  )) != 0 ) {
       printf1("ERROR: post-syscall setup failed (%d)\n", rc);
       goto EXIT;
     }
   
    EXIT:
     printf5( "Returning from test\n" );
     return rc;
   }
