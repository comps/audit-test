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
  **  FILE       : test_lremovexattr_einval.c
  **
  **  PURPOSE    : To test the lremovexattr library call auditing.
  **
  **  DESCRIPTION: The test_lremovexattr_einval() function builds into the
  **  laus_test framework to verify that the Linux Audit System
  **  accurately logs both successful and erroneous execution of the
  **  "lremovexattr" system call.
  **
  **  In the successful case, this function:
  **   1) Creates a temporary file
  **   2) Sets the user.mime_type extended attribute of the newly
  **      created temporary file to ``text/plain''
  **   3) Calls lremovexattr on the newly added attribute of the
  **      temporary file
  **   4) Verifies the success result.
  **
  **  The successful case passes a valid path and name to the
  **  lremovexattr call, thus satisfying the conditions as given in the
  **  man page for lremovexattr for a success result.
  **  
  **  In the erroneous case, this function:
  **   1) Calls lremovexattr with path=NULL
  **   2) Verifies the error result.
  **      
  **  The erroneous case causes an EFAULT as detailed in the man page in
  **  stat(2) by passing an invalid address (NULL) to lremovexattr.
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
 
 int test_lremovexattr_einval(laus_data* dataPtr) {
   
   int rc = 0;
   int exp_errno = EFAULT;
   size_t size;
 
   char* path = NULL;
   char* name = "user.mime_type";
 
   // Set the syscall-specific data
   printf5( "Setting laus_var_data.syscallData.code to %d\n", AUDIT_lremovexattr );
   dataPtr->laus_var_data.syscallData.code = AUDIT_lremovexattr;
   
   /**
    * Do as much setup work as possible right here
    */
   size = sizeof( XATTR_TEST_VALUE );
   if( dataPtr->successCase ) {     // Set up for success
       // Create the target file
       if( ( rc = createTempFile( &path, ( S_IRWXU | S_IRWXG | S_IRWXO ),
				  dataPtr->msg_euid, dataPtr->msg_egid ) ) == -1 ) {
	   printf1( "ERROR: Cannot create file %s\n", path );
	   goto EXIT;
       }
       if( ( rc = setxattr( path, name, "test/plain", strlen( XATTR_TEST_VALUE ), XATTR_CREATE ) ) == -1 ) {
	   printf(" Error setting attribute [%s]: errno=%i\n", name, errno );
	   goto EXIT_CLEANUP;
       }
   } else {// Set up for error       
       path = NULL;
   }
   
   // Set up audit argument buffer
   if( ( rc = auditArg2( dataPtr,
 		      ( path == NULL ? AUDIT_ARG_NULL : AUDIT_ARG_PATH ),
 		      ( path == NULL ? 0 : strlen( path ) ), path,
 		      AUDIT_ARG_STRING, strlen( name ), name ) ) != 0 ) {
     printf1( "Error setting up audit argument buffer\n" );
     goto EXIT_CLEANUP;
   }
 
   // Do pre-system call work
   preSysCall( dataPtr );
 
   // Execute system call
   dataPtr->laus_var_data.syscallData.result = syscall( __NR_lremovexattr, path, name );
 
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

