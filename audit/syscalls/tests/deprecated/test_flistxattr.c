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
  **  FILE       : test_flistxattr.c
  **
  **  PURPOSE    : To test the flistxattr library call auditing.
  **
  **  DESCRIPTION: The test_flistxattr() function builds into the
  **  laus_test framework to verify that the Linux Audit System
  **  accurately logs both successful and erroneous execution of the
  **  "flistxattr" system call.
  **
  **  In the successful case, this function:
  **   1) Creates a temporary file
  **   2) Opens the temporary file
  **   2) Calls flistxattr on the file descriptor
  **   3) Verifies the success result.
  **
  **  The successful case passes a valid file descriptor, list, and
  **  size to the flistxattr call, thus satisfying the conditions as
  **  given in the man page for flistxattr for a success result.
  **  
  **  In the erroneous case:
  **   1) Creates a temporary file
  **   2) Sets the test xattr
  **   3) Opens the temporary file
  **   4) Calls flistxattr on the file descriptor with a too small list buffer
  **
  **  The erroneous case passes a valid filedescriptor and a too small list
  **  buffer and size to the flistxattr call, thus causing an ERANGE error.
  **      
  **
  **  HISTORY    :
  **    07/03 Originated by Michael A. Halcrow <mike@halcrow.us>
  **    07/03 Furthered by Kylene J. Smith <kylene@us.ibm.com>
  **    03/04 Added exp_errno variable by D. Kent Soper <dksoper@us.ibm.com>
  **
  **********************************************************************/

#ifndef NOXATTR
 
 #include "syscalls.h"
 #include <sys/types.h>
 #include <attr/xattr.h>
 
 int test_flistxattr(laus_data* dataPtr) {
   
   int rc = 0;
   int exp_errno = ERANGE;
   char* path = NULL;
 
   int filedes;
   char* name = "user.mime_type";
   char* list = NULL;
   size_t size = 0;
 
   // Set the syscall-specific data
   printf5( "Setting laus_var_data.syscallData.code to %d\n", __NR_flistxattr );
   dataPtr->laus_var_data.syscallData.code = __NR_flistxattr;
   // Create the target file
   if( ( rc = createTempFile( &path, ( S_IRWXU | S_IRWXG | S_IRWXO ),
			      dataPtr->msg_euid, dataPtr->msg_egid ) ) == -1 ) {
       printf1( "ERROR: Cannot create file %s\n", path );
       goto EXIT;
   }
   // Do as much setup work as possible right here
   if( dataPtr->successCase ) {       // Set up for success
       
   } else {    // Set up for error
       list = (char*)malloc(sizeof(char));
       size = 1;
       if( ( rc = setxattr( path, name, "test/plain", strlen( XATTR_TEST_VALUE ), XATTR_CREATE ) ) == -1 ) {
	   printf1( "Error setting attribute [%s]: errno=%i\n", name, errno );
	   goto EXIT_CLEANUP_UNLINK;
       }
   }
   // Open the file
   if( ( filedes = rc = open( path, O_RDWR ) ) == -1 ) {
       printf1( "Unable to open file [%s] for read/write access\n", path );
       goto EXIT_CLEANUP_UNLINK;
   }
   
   // Set up audit argument buffer
   if( ( rc = auditArg3( dataPtr,
			 AUDIT_ARG_PATH, strlen( path ), path,
			 (dataPtr->successCase ? AUDIT_ARG_NULL : AUDIT_ARG_POINTER), 
			 0, list,
			 AUDIT_ARG_IMMEDIATE, sizeof( size_t ), &size ) ) != 0 ) {
       printf1( "Error setting up audit argument buffer\n" );
       goto EXIT_CLEANUP_CLOSE;
   }
 
   // Do pre-system call work
   preSysCall( dataPtr );
 
   // Execute system call
   dataPtr->laus_var_data.syscallData.result = syscall( __NR_flistxattr, filedes, list, size );
 
   // Do post-system call work
   postSysCall( dataPtr, errno, -1, exp_errno );
 
  EXIT_CLEANUP_CLOSE:
     // Clean up from success case setup
     if( (  close( filedes ) ) == -1 ) {
       printf1( "Error closing file descriptor %d\n", filedes );
       goto EXIT_CLEANUP_UNLINK;
     }
 
  EXIT_CLEANUP_UNLINK:
   // Clean up from success case setup
       if( (  unlink( path ) ) == -1 ) {
	   printf1( "Error unlinking file %s\n", path );
	   goto EXIT;
       }
 
  EXIT:
   if ( path )
     free( path );
   printf5( "Returning from test\n" );
   return rc;
 }

#endif

