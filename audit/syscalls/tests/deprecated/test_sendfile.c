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
   **  FILE       : test_sendfile.c
   **
   **  PURPOSE    : To test the sendfile library call auditing.
   **
   **  DESCRIPTION: The test_sendfile() function builds into the
   **  laus_test framework to verify that the Linux Audit System
   **  accurately logs both successful and erroneous execution of the
   **  "sendfile" system call.
   **
   **
   **  In the successful case, this function:
   **   1) Generates a 2 temporary filenames
   **   2) Creates the 2 temporary files
   **   3) Opens the temporary files with open
   **   4) Makes the sendfile syscall 
   **   9) Closes and removes the temporary files
   **
   **  The successful case creates and opens a 2 temporary files. It then
   **  uses the sendfile system call to copy 1 byte of data from the first
   **  file to the second.  This is expected to succeed because the files
   **  were created and initialized right before the syscall 
   **  
   **  In the erroneous case, this function:
   **  1) Attempt the sendfile system call with file descriptors -1.
   **      
   **  The erroneous case invokes an error by passing sendfile() file 
   **  descriptors of -1.  According to the man page, an invalid file 
   **  descriptor passed to sendfile(), generates EBADF.
   **
   **  HISTORY    :
   **    06/03 Originated by Kylene J. Smith <kylene@us.ibm.com>
   **
   **********************************************************************/
   
   #include "syscalls.h"
   #include <unistd.h>
   #include <sys/sendfile.h>
   #include <sys/types.h>
   #include <sys/stat.h>
   #include <fcntl.h>
   
   int test_sendfile(laus_data* dataPtr) {
     
     int rc = 0;
     int exp_errno = EBADF;
     char* path_in = NULL;
     char* path_out = NULL;
     int fd_in, fd_out;
     off_t offset = 1;
     size_t count = 1;
   
     
     // Set the syscall-specific data
     printf5( "Setting laus_var_data.syscallData.code to %d\n", __NR_sendfile );
     dataPtr->laus_var_data.syscallData.code = __NR_sendfile;
     
     if( dataPtr->successCase ) {
         // dynamically create test directory
         if (rc = (createTempFile(&path_in, S_IRWXU | S_IRWXG | S_IRWXO,
   			       dataPtr->msg_euid, dataPtr->msg_egid)) == -1) {
   	  printf1("ERROR: Cannot create path_in %s\n", path_in);
   	  goto EXIT;
         }
         if ( (fd_in= open(path_in, O_RDONLY) ) < 0 ) { //open test directory
   	  printf1("ERROR: Cannot open path_in %s\n", path_in);
   	  goto EXIT_IN;
         }
         if (rc = (createTempFile(&path_out, S_IRWXU | S_IRWXG | S_IRWXO,

   			       dataPtr->msg_euid, dataPtr->msg_egid)) == -1) {
   	  printf1("ERROR: Cannot create path_out %s\n", path_out);
   	  goto EXIT_IN_OPEN;
         }
         if ( (fd_out= open(path_out, O_WRONLY) ) < 0 ) { //open test directory
   	  printf1("ERROR: Cannot open path_out %s\n", path_out);
   	  goto EXIT_OUT;
         }
   
         
      } else {
         path_in = NULL;
         path_out = NULL; 
         fd_in = -1;
         fd_out = -1;
     }
     printf5( "Generated and opened  %s fd = %d %s fd = %d\n", 
   	   path_in, fd_in, path_out, fd_out );      
    
     // Set up audit argument buffer
     if( ( rc = auditArg4( dataPtr, 

			   (dataPtr->successCase ? AUDIT_ARG_PATH : AUDIT_ARG_ERROR), 
			   (dataPtr->successCase ? strlen(path_out) : sizeof(int)), 
			   (dataPtr->successCase ? path_out : (void*)&exp_errno ),

			   (dataPtr->successCase ? AUDIT_ARG_PATH : AUDIT_ARG_ERROR), 
			   (dataPtr->successCase ? strlen(path_in) : sizeof(int)), 
			   (dataPtr->successCase ? path_in : (void*)&exp_errno ),

			   AUDIT_ARG_POINTER, sizeof (off_t), &offset,

			   AUDIT_ARG_IMMEDIATE, sizeof( size_t), &count 

			   ) ) != 0 ) {
         printf1( "Error setting up audit argument buffer\n" );
         goto EXIT_OUT_OPEN;
         
     }
     
     // Do pre-system call work
     if ( (rc = preSysCall( dataPtr )) != 0 ) {
       printf1("ERROR: pre-syscall setup failed (%d)\n", rc);
       goto EXIT_CLEANUP;
     }
     
     // Execute system call
     dataPtr->laus_var_data.syscallData.result = syscall( __NR_sendfile, fd_out, fd_in, &offset, count );
     
     // Do post-system call work
     if ( (rc = postSysCall(  dataPtr, errno, -1, exp_errno  )) != 0 ) {
       printf1("ERROR: post-syscall setup failed (%d)\n", rc);
       goto EXIT_CLEANUP;
     }
   
     /**
      * Do cleanup work here
      */
  
  EXIT_CLEANUP:
  
    EXIT_OUT_OPEN:
     // close the temporary out file
     if( dataPtr->successCase && close( fd_out ) == -1 ) {
         printf1( "Error closinging path_out %s: errno=%i\n", path_out, errno );
     }
    EXIT_OUT:
     // remove the temporary out file
     if( dataPtr->successCase && unlink( path_out ) == -1 ) {
         printf1( "Error removing path_out %s: errno=%i\n", path_out, errno );
     }
     
    EXIT_IN_OPEN:
     // close the temporary in file
     if( dataPtr->successCase && close( fd_in ) == -1 ) {
         printf1( "Error closinging path_in %s: errno=%i\n", path_in, errno );
     }
    EXIT_IN:
     // remove the temporary in file
     if( dataPtr->successCase && unlink( path_in ) == -1 ) {
         printf1( "Error removing path_in %s: errno=%i\n", path_in, errno );
     }
     
    EXIT:
     if (path_in)
       free(path_in);
     if (path_out)
       free(path_out);
     printf5( "Returning from test\n" );
     return rc;
   }
   
