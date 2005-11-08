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
   **  FILE       : test_rmdir_efault.c
   **
   **  PURPOSE    : To test the rmdir library call auditing.
   **
   **  DESCRIPTION: The test_rmdir() function builds into the
   **  laus_test framework to verify that the Linux Audit System
   **  accurately logs both successful and erroneous execution of the
   **  "rmdir" system call.
   **
   **  In the successful case, this function:
   **   1) Creates the temporary directory
   **   2) Makes the rmdir syscall on the temporary directory
   **
   **  The successful case removes the directory in the specified in the
   **  path.  Since the directory is created immediately before removing
   **  to it, we can expect rmdir() to successfully execute.
   **  
   **  In the erroneous case, this function:
   **   1) Makes the rmdir syscall with a NULL parameter
   **      
   **  The erroneous case invokes an error by passing rmdir() a NULL
   **  pointer.  According to the man page, when a pointer that is
   **  outside the accessible address space is passed to rmdir(), EFAULT
   **  is generated.
   **
   **  HISTORY    :
   **    06/03 Originated by Michael A. Halcrow <mike@halcrow.us>
   **    06/03 Furthered by Dustin Kirkland <k1rkland@us.ibm.com>
   **    03/04 Added exp_errno variable by D. Kent Soper <dksoper@us.ibm.com>
   **
   **********************************************************************/
   
   #include "syscalls.h"
   #include <unistd.h>
   
   int test_rmdir_efault(laus_data* dataPtr) {
     
    
     int rc = 0;
     int exp_errno = EFAULT;
   
     char* path = NULL;
     
     
     // Set the syscall-specific data
     printf5( "Setting laus_var_data.syscallData.code to %d\n", AUDIT_rmdir );
     dataPtr->laus_var_data.syscallData.code = AUDIT_rmdir;
     
     if( dataPtr->successCase ) {
       // dynamically create test directory
       if (rc = (createTempDir(&path, S_IRWXU | S_IRWXG | S_IRWXO,
                                dataPtr->msg_euid, dataPtr->msg_egid)) == -1) {
         printf1("ERROR: Cannot create dir %s\n", path);
         goto EXIT;
       }
     } else {
       path = NULL;
     }
     printf5( "Generated directory %s\n", path );
   
     // Set up audit argument buffer
     if( ( rc = auditArg1( dataPtr,
   		 ( dataPtr->successCase ? AUDIT_ARG_PATH : AUDIT_ARG_NULL ),
   		 ( dataPtr->successCase ? strlen( path ) : 0), path ) ) != 0 ) {
       printf1( "Error setting up audit argument buffer\n" );
       goto EXIT;
     }
   
   
     // Do pre-system call work
     if ( (rc = preSysCall( dataPtr )) != 0 ) {
       printf1("ERROR: pre-syscall setup failed (%d)\n", rc);
       goto EXIT_CLEANUP;
     }
   
     // Execute system call
     dataPtr->laus_var_data.syscallData.result = syscall( __NR_rmdir, path );
   
     // Do post-system call work
     if ( (rc = postSysCall(  dataPtr, errno, -1, exp_errno  )) != 0 ) {
       printf1("ERROR: post-syscall setup failed (%d)\n", rc);
       goto EXIT_CLEANUP;
     }
   
   
    EXIT_CLEANUP:
     /**
      * Do cleanup work here
      */
   
    EXIT:
     if (path)
       free(path);
     printf5( "Returning from test\n" );
     return rc;
   }
