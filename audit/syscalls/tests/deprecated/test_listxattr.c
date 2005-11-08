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
  **  FILE       : test_listxattr.c
  **
  **  PURPOSE    : To test the listxattr library call auditing.
  **
  **  DESCRIPTION: The test_listxattr() function builds into the
  **  laus_test framework to verify that the Linux Audit System
  **  accurately logs both successful and erroneous execution of the
  **  "listxattr" system call.
  **
  **  In the successful case, this function:
  **   1) Creates a temporary file
  **   2) Calls listxattr on the temporary file
  **   3) Verifies the success result.
  **
  **  The successful case passes a valid path, list, and size to the
  **  listxattr call, thus satisfying the conditions as given in the man
  **  page for listxattr for a success result.
  **  
  **  In the erroneous case, this function:
  **   1) Calls listxattr with path=NULL
  **   2) Verifies the error result.
  **      
  **  The erroneous case causes an EFAULT as detailed in the man page in
  **  stat(2) by passing an invalid address (NULL) to listxattr.
  **
  **  HISTORY    :
  **    06/03 Originated by Michael A. Halcrow <mike@halcrow.us>
  **    03/04 Added exp_errno variable by D. Kent Soper <dksoper@us.ibm.com>
  **
  **********************************************************************/

#ifndef NOXATTR
 
 #include "syscalls.h"
 #include <sys/types.h>
 #include <attr/xattr.h>
 
 int test_listxattr(laus_data* dataPtr) {
   
   int rc = 0;
   int exp_errno = EFAULT;
 
   char* path = NULL;
   char* list = NULL;
   size_t size;
 
   // Set the syscall-specific data
   printf5( "Setting laus_var_data.syscallData.code to %d\n", __NR_listxattr );
   dataPtr->laus_var_data.syscallData.code = __NR_listxattr;
   
   /**
    * Do as much setup work as possible right here
    */
   size = 0;
   list = NULL;
   if( dataPtr->successCase ) {
     // Set up for success
     dataPtr->msg_euid = 0;
     dataPtr->msg_egid = 0;
     dataPtr->msg_fsuid = 0;
     dataPtr->msg_fsgid = 0;
     // Create the target file
     if( ( rc = createTempFile( &path, ( S_IRWXU | S_IRWXG | S_IRWXO ),
 			     dataPtr->msg_euid, dataPtr->msg_egid ) ) == -1 ) {
       printf1( "ERROR: Cannot create file %s\n", path );
       goto EXIT;
     }
   } else {
     // Set up for error
     path = NULL;
   }
 
   // Set up audit argument buffer
   if( ( rc = auditArg3( dataPtr,
 		      ( path == NULL ? AUDIT_ARG_NULL : AUDIT_ARG_PATH ),
 		      ( path == NULL ? 0 : strlen( path ) ), 
 		      path,
 		      AUDIT_ARG_NULL, 0, list,
 		      AUDIT_ARG_IMMEDIATE, sizeof( size_t ), &size ) ) != 0 ) {
     printf1( "Error setting up audit argument buffer\n" );
     goto EXIT_CLEANUP;
   }
 
   // Do pre-system call work
   preSysCall( dataPtr );
 
   // Execute system call
   dataPtr->laus_var_data.syscallData.result = syscall( __NR_listxattr, path, list, size );
 
   // Do post-system call work
   postSysCall( dataPtr, errno, -1, exp_errno );
 
  EXIT_CLEANUP:
   /**
    * Do cleanup work here
    */
   if( dataPtr->successCase ) {
     // Clean up from success case setup
     if( (  unlink( path ) ) == -1 ) {
       printf1( "Error unlinking file %s\n", path );
       goto EXIT;
     }
   }
 
  EXIT:
   if ( path )
     free( path );
   printf5( "Returning from test\n" );
   return rc;
 }

#endif
