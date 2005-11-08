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
   **  FILE       : test_mkdir_efault.c
   **
   **  PURPOSE    : To test the mkdir library call auditing.
   **
   **  DESCRIPTION: The test_mkdir_efault() function builds into the
   **  laus_test framework to verify that the Linux Audit System
   **  accurately logs both successful and erroneous execution of the
   **  "mkdir" system call.
   **
   **  In the successful case, this function:
   **   1) Generates a random new temporary directory name
   **   2) Sets the euid to the test user
   **   3) Performs the mkdir system call with the temporary directory
   **      name
   **   4) Sets the euid to the superuser
   **   5) Tests the syscall return value for successful result
   **   6) Removes the temporary directory.
   **
   **  The successful case generates a new random temporary directory
   **  name and calls mkdir() with that name.  According to the man page
   **  (and assuming the mksfile() function call returns a unique,
   **  nonexistent filename), mkdir() should return success.
   **  
   **  In the erroneous case, this function:
   **   1) Clears the audit trail
   **   2) Sets the euid to the test user
   **   3) Performs the mkdir system call with NULL for the temporary
   **      directory name pointer
   **   4) Sets the euid to the superuser
   **   5) Tests the syscall return value for erroneous result
   **      
   **  The erroneous case, according to the man page for mkdir(), should
   **  cause an EFAULT error result, due to the fact that a NULL pointer
   **  is outside the accessible address space.
   **
   **  HISTORY    :
   **    06/03 Originated by Michael A. Halcrow <mike@halcrow.us>
   **    03/04 Added exp_errno variable by D. Kent Soper <dksoper@us.ibm.com>
   **
   **********************************************************************/
   
   #include "syscalls.h"
   #include <sys/stat.h>
   #include <sys/types.h>
   
   int test_mkdir_efault(laus_data* dataPtr) {
    
     int rc = 0;
     int exp_errno = EFAULT;
   
     char* path = NULL;
     int mode;
     
     
     // Set the syscall-specific data
     printf5( "Setting laus_var_data.syscallData.code to %d\n", AUDIT_mkdir );
     dataPtr->laus_var_data.syscallData.code = AUDIT_mkdir;
     
     /**
      * Do as much setup work as possible right here
      */
     // set the mode
     mode = S_IRWXU | S_IRWXG | S_IRWXO;
     if( dataPtr->successCase ) {
       // dynamically create temp file, and delete it real quick
       if ((rc = createTempFile(&path, S_IRWXU | S_IRWXG | S_IRWXO,
                                dataPtr->msg_euid, dataPtr->msg_egid)) == -1) {
         printf1("ERROR: Cannot create file %s\n", path);
         goto EXIT;
       }
       if ((rc = unlink(path)) != 0) {
         printf1("ERROR: Unable to remove file %s: errno=%i\n", path, errno);
         goto EXIT;
       }
       printf5( "Generated directory name %s\n", path );
     } else {
       path = NULL;
     }
     
     if( ( rc = auditArg2( dataPtr,
                    dataPtr->successCase ? AUDIT_ARG_PATH : AUDIT_ARG_NULL,
   		 dataPtr->successCase ? strlen( path ): 0, path,
                    AUDIT_ARG_IMMEDIATE, sizeof( int ), &mode ) ) != 0 ) {
       printf1( "Error setting up audit argument buffer\n" );
       goto EXIT;
     }
   
     // Do pre-system call work
     if ( (rc = preSysCall( dataPtr )) != 0 ) {
       printf1("ERROR: pre-syscall setup failed (%d)\n", rc);
       goto EXIT_CLEANUP;
     }
   
     // Execute system call
     dataPtr->laus_var_data.syscallData.result = syscall( __NR_mkdir, path, mode );
   
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
       if( rmdir( path ) == -1 ) {
         printf1( "Error removing directory %s during cleanup\n", path );
         goto EXIT;
       }
     }
   
    EXIT:
     if ( path )
       free( path );
     printf5( "Returning from test\n" );
     return rc;
   }
