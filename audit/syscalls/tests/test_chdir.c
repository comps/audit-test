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
   **  FILE       : test_chdir.c
   **
   **  PURPOSE    : To test the chdir library call auditing.
   **
   **  DESCRIPTION: The test_chdir() function builds into the
   **  laus_test framework to verify that the Linux Audit System
   **  accurately logs both successful and erroneous execution of the
   **  "chdir" system call.
   **
   **  In the successful case, this function:
   **   1) Generates a temporary directory name
   **   2) Creates the temporary directory
   **   3) Sets the euid to the test user
   **   4) Makes the chdir syscall to the temporary directory
   **   5) Sets the euid to the superuser
   **   6) Tests the result of the system call to determine whether or
   **      not the call was successful
   **   7) Changes to the ".." directory
   **   8) Removes the temporary directory.
   **
   **  The successful case changes the current directory to that
   **  specified in the path.  Since the directory is created immediately
   **  before changing to it, we can expect chdir() to successfully
   **  execute.
   **  
   **  In the erroneous case, this function:
   **      
   **  The erroneous case invokes the EACCES error by trying to chdir
   **  to a 700 directory as a non-root, non-owner user.
   **
   **  HISTORY    :
   **    06/03 Originated by Michael A. Halcrow <mike@halcrow.us>
   **    11/03 Furthered by Dustin Kirkland <k1rkland@us.ibm.com>
   **    03/04 Added exp_errno variable by D. Kent Soper <dksoper@us.ibm.com>
   **    05/04 Updates to supress compiler warnings by Kimberly D. Simon <kdsimon@us.ibm.com>
   **
   **********************************************************************/
   
   #include "syscalls.h"
   #include <unistd.h>
   
   int test_chdir(laus_data* dataPtr) {
     
    
     int rc = 0;
     int exp_errno =EACCES;
     char* path = NULL;
     
     // Set the syscall-specific data
     printf5( "Setting laus_var_data.syscallData.code to %d\n", AUDIT_chdir );
     dataPtr->laus_var_data.syscallData.code = AUDIT_chdir;
     
     // dynamically create test directory
     if ((rc = (createTempDir(&path, S_IRWXU,
                              dataPtr->msg_euid, dataPtr->msg_egid)) == -1)) {
       printf1("ERROR: Cannot create dir %s\n", path);
       goto EXIT;
     } 
  
     if ( ! dataPtr->successCase ) {
       dataPtr->msg_ruid = dataPtr->msg_euid = dataPtr->msg_fsuid = helper_uid;
     }

     printf5( "Generated directory %s\n", path );
   
     // Set up audit argument buffer
     if( ( rc = auditArg1( dataPtr,
 			  ( AUDIT_ARG_PATH ), 
 			  ( strlen( path ) ), 
 			  path ) ) != 0 ) {
       printf1( "Error setting up audit argument buffer\n" );
       goto EXIT_CLEANUP;
   
     }
   
     // Do pre-system call work
     if ( (rc = preSysCall( dataPtr )) != 0 ) {
       printf1("ERROR: pre-syscall setup failed (%d)\n", rc);
       goto EXIT_CLEANUP;
     }
   
     // Execute system call
     dataPtr->laus_var_data.syscallData.result = syscall( __NR_chdir, path );
   
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
       // chdir out of the directory we are about to nuke
       if( chdir( cwd ) == -1 ) {
         printf1( "Error executing chdir(\"%s\"): errno=%i\n", cwd, errno );
         goto EXIT;
       }
     }
     // remove the temporary directory
     if( rmdir( path ) == -1 ) {
       printf1( "Error removing directory %s: errno=%i\n", path, errno );
       goto EXIT;
     }
   
    EXIT:
     if (path) 
       free (path);
     printf5( "Returning from test\n" );
     return rc;
   }
