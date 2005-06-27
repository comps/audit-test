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
   **  FILE       : test_fchdir.c
   **
   **  PURPOSE    : To test the fchdir library call auditing.
   **
   **  DESCRIPTION: The test_fchdir() function builds into the
   **  laus_test framework to verify that the Linux Audit System
   **  accurately logs both successful and erroneous execution of the
   **  "fchdir" system call.
   **
   **  In the successful case, this function:
   **   1) Generates a temporary directory name
   **   2) Creates the temporary directory
   **   3) Open the temporary directory with opendir.
   **   4) Clears the audit trail
   **   5) Sets the euid to the test user
   **   6) Makes the fchdir syscall to the temporary directory
   **   7) Sets the euid to the superuser
   **   8) Tests the result of the system call to determine whether or
   **      not the call was successful
   **   8) Changes to the ".." directory
   **   9) Removes the temporary directory.
   **
   **  The successful case changes the current directory to that
   **  specified in the path via an open file descriptor.  Since 
   **  the directory is created immediately before changing to it, 
   **  we can expect fchdir() to successfully execute.
   **  
   **  In the erroneous case, this function:
   **   1) Clears the audit trail
   **   2) Sets the euid to the test user
   **   3) Makes the fchdir syscall with a fd opened to a file
   **   4) Sets the euid to the superuser
   **   5) Tests the result of the system call to determine whether or
   **      not the call was unsuccessful.
   **      
   **  The erroneous case invokes an error by passing fchdir() a file 
   **  descriptor of an open file rather than a directory.  According to 
   **  the man page, an file passed to fchdir(), generates ENOTDIR. We 
   **  tried to use -1 to cause EBADF.  Did not generate an audit record
   **  suspect that this error is handled above the kernel.
   **  
   **
   **  HISTORY    :
   **    06/03 Originated by Kylene J. Smith <kylene@us.ibm.com>
   **    03/04 Added exp_errno variable by D. Kent Soper <dksoper@us.ibm.com>
   **    05/04 Updates to suppress compiler warning by Kimberly D. Simon <kdsimon@us.ibm.com>
   **
   **********************************************************************/
   
   #include "includes.h"
   #include "syscalls.h"
   #include <dirent.h>
   
   int test_fchdir(laus_data* dataPtr) {
     
     int rc = 0;
     int exp_errno = ENOTDIR;
     char* path = NULL;
     int fd;
     DIR* dir = NULL;
     
     // Set the syscall-specific data
     printf5( "Setting laus_var_data.syscallData.code to %d\n", AUDIT_fchdir );
     dataPtr->laus_var_data.syscallData.code = AUDIT_fchdir;
     
     if( dataPtr->successCase ) {
         // dynamically create test directory
         if ((rc = (createTempDir(&path, S_IRWXU | S_IRWXG | S_IRWXO,
   			      dataPtr->msg_euid, dataPtr->msg_egid)) == -1)) {
   	  printf1("ERROR: Cannot create dir %s\n", path);
   	  goto EXIT;
         } 
         if ( (dir = opendir (path) ) == NULL ) { //open test directory
   	  printf1("ERROR: Cannot open dir %s\n", path);
   	  goto EXIT_CLEANUP;
         }
         if ( ( fd = dirfd(dir) ) < 0 ) {
   	  printf1("ERROR: Cannot get valid file descriptor for %s\n", path);
   	  goto EXIT_CLEANUP;
         }
      } else {
	  if ((rc = (createTempFile(&path, S_IRWXU | S_IRWXG | S_IRWXO,
				   dataPtr->msg_euid, dataPtr->msg_egid)) == -1)) {
   	  printf1("ERROR: Cannot create file%s\n", path);
   	  goto EXIT;
         }
	  if ( ( fd = open(path, O_RDONLY) ) < 0 ) {
   	  printf1("ERROR: Cannot get valid file descriptor for %s\n", path);
   	  goto EXIT_CLEANUP;
         }
	  
      }
     printf5( "Generated and opened path %s fd = %d\n", path, fd );      
    
     // Set up audit argument buffer
     if( ( rc = auditArg1( dataPtr,
			 AUDIT_ARG_PATH, strlen( path ), path ) ) != 0 ) {
         printf1( "Error setting up audit argument buffer\n" );
         goto EXIT_CLEANUP_OPENED;
     }
     
     // Do pre-system call work
     if ( (rc = preSysCall( dataPtr )) != 0 ) {
       printf1("ERROR: pre-syscall setup failed (%d)\n", rc);
       goto EXIT_CLEANUP;
     }
     
     // Execute system call
     dataPtr->laus_var_data.syscallData.result = syscall( __NR_fchdir, fd );
     
     // Do post-system call work
     if ( ( rc = postSysCall( dataPtr, errno, -1, exp_errno ) ) != 0 ) {
       printf1("ERROR: post-syscall setup failed (%d)\n", rc);
       goto EXIT_CLEANUP;
     }
     
    EXIT_CLEANUP_OPENED:
     if( dataPtr->successCase && closedir( dir ) == -1 ) {
         printf1("Error closing directory %s: errno=%i\n", path, errno );
     }
    EXIT_CLEANUP:
     /**
      * Do cleanup work here
      */
     if( dataPtr->successCase ) {
         // chdir out of the directory we are about to nuke
         if( chdir( cwd ) == -1 ) {
   	  printf1( "Error executing chdir(\"%s\"): errno=%i\n", cwd, errno );
         }
         // remove the temporary directory
         if( rmdir( path ) == -1 ) {
   	  printf1( "Error removing directory %s: errno=%i\n", path, errno );
         }
     }
     else {
	 if ( unlink( path ) == -1 ) {
	     printf1( "Error removing file %s: errno=%i\n", path, errno );
         }
     }   
    EXIT:
     if ( path )
       free( path );
     printf5( "Returning from test\n" );
     return rc;
   }
