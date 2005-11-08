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
  **  FILE       : test_create_module.c
  **
  **  PURPOSE    : To test the create_module library call auditing.
  **
  **  DESCRIPTION: The test_create_module() function builds into the
  **  laus_test framework to verify that the Linux Audit System
  **  accurately logs both successful and erroneous execution of the
  **  "create_module" system call.
  **
  **  In the successful case, this function:
  **   1) Executes create_module() on a unique, nonexistent module name
  **   2) Tests the result of the syscall against the expected result
  **      for the successful case
  **   3) Executes delete_module() on the module name.
  **
  **  The successful case uses a unique, nonexistent module name to
  **  avoid a naming conflict with another module.  The name is chosen
  **  in such a manner so as to make a collision with another module
  **  highly improbable.  As the superuser, the create_module() call
  **  should succeed, according to the information given on the man
  **  page for the syscall.
  **  
  **  In the erroneous case, this function:
  **   1) Sets the effective user ID to the test user
  **   2) Executes create_module() on a unique, nonexistent module name
  **   3) Sets the effective user ID to the superuser
  **   4) Tests the result of the syscall against the expected result
  **      for the erroneous case.
  **      
  **  The erroneous case sets the effective user ID to the test user.
  **  According to the create_module() man page, the syscall is only
  **  open to the superuser.  We can thus expect an EPERM error code as
  **  a result.
  **
  **  HISTORY    :
  **    06/02 Originated by Michael A. Halcrow <mike@halcrow.us>
  **    03/04 Added exp_errno variable by D. Kent Soper <dksoper@us.ibm.com>
  **
  **********************************************************************/
   
 #include "syscalls.h"
 #include <asm/module.h>
 #include <asm/page.h>
   
 int test_create_module(laus_data* dataPtr) {
 
   int rc = 0;
   int exp_errno = EPERM;

   char* uniqueNonexistentModuleName = "f6e5c496f1fe799";
   int size = PAGE_SIZE * 3; // Not too small; not too large

   delete_module( uniqueNonexistentModuleName );
     
   // Set the syscall-specific data
   dataPtr->laus_var_data.syscallData.code = __NR_create_module;
   if ( dataPtr->successCase ) {
     // only superuser can create modules
     dataPtr->msg_euid = 0;
     dataPtr->msg_egid = 0;
     dataPtr->msg_fsuid = 0;
     dataPtr->msg_fsgid = 0;
   }
   // Set up audit argument buffer
   if( ( rc = auditArg2( dataPtr,
 		      AUDIT_ARG_STRING, strlen(uniqueNonexistentModuleName), uniqueNonexistentModuleName,
 		      AUDIT_ARG_IMMEDIATE, sizeof( int ), &size ) ) != 0 ) {
     printf1( "Error setting up audit argument buffer\n" );
     goto EXIT;
   }
   
   // Do pre-system call work
   if ( (rc = preSysCall( dataPtr )) != 0 ) {
     printf1("ERROR: pre-syscall setup failed (%d)\n", rc);
     goto EXIT_CLEANUP;
   }
   
   // Execute system call
   dataPtr->laus_var_data.syscallData.result = syscall( __NR_create_module, uniqueNonexistentModuleName, size );
   
   // Do post-system call work
   if ( (rc = postSysCall(  dataPtr, errno, -1, exp_errno  )) != 0 ) {
     printf1("ERROR: post-syscall setup failed (%d)\n", rc);
     goto EXIT_CLEANUP;
   }
     
  EXIT_CLEANUP:
   if( dataPtr->successCase ) {
     delete_module( uniqueNonexistentModuleName );
   }
   
  EXIT:
   return rc;
 }
