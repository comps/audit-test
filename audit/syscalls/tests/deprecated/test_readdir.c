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
   **  FILE       : test_readdir.c
   **
   **  PURPOSE    : To test the readdir library call auditing.
   **
   **  DESCRIPTION: The test_readdir() function builds into the
   **  laus_test framework to verify that the Linux Audit System
   **  accurately logs both successful and erroneous execution of the
   **  "readdir" system call.
   **
   **  NOTE: man(2) vs man(3) shows to different calls.  This is test is
   **        using the one described in man(2).  Have to use syscall because
   **        man (2) readdir description caused segmentation faults.
   **
   **  In the successful case, this function:
   **   1) Generates a temporary directory name
   **   2) Creates the temporary directory
   **   3) Open the temporary directory with open.
   **   4) Makes the readdir syscall on the temporary directory
   **   5) Closes and removes the temporary directory.
   **
   **  The successful case creates and opens a temp. directory and does
   **  a readdir() on that directory.  It is expected to succeed because
   **  the directory was just created. 
   **  
   **  In the erroneous case, this function:
   **  1) Attempt the readdir on fd -1.
   **      
   **  The erroneous case invokes an error by passing readdir() a file 
   **  descriptor of -1.  According to the man page, an invalid file 
   **  descriptor passed to readdir(), generates EBADF.
   **
   **  HISTORY    :
   **    06/03 Originated by Kylene J. Smith <kylene@us.ibm.com>
   **
   **********************************************************************/
#if !defined(__S390) && !defined(__PPC) && !defined(__X86_64)
   
   #include "syscalls.h"
   #include <unistd.h>
   #include <linux/types.h>
   #include <linux/dirent.h>
   #include <linux/unistd.h>
   #include <sys/types.h>
   #include <sys/stat.h>
   #include <fcntl.h>
   
   int test_readdir(laus_data* dataPtr) {
   
     int rc = 0;
     int exp_errno = EBADF;
     char* path = NULL;
     int fd;
     unsigned int count = 1;
     struct dirent dir;
     
     // Set the syscall-specific data
     printf5( "Setting laus_var_data.syscallData.code to %d\n", __NR_readdir );
     dataPtr->laus_var_data.syscallData.code = __NR_readdir;
     
     if( dataPtr->successCase ) {      // dynamically create test directory
         if (rc = (createTempDir(&path, S_IRWXU | S_IRWXG | S_IRWXO,
   			      dataPtr->msg_euid, dataPtr->msg_egid)) == -1) {
   	  printf1("ERROR: Cannot create dir %s\n", path);
   	  goto EXIT;
   	  } 
         if ( (fd = open (path, O_RDONLY) ) < 0 ) { //open test directory
   	  printf1("ERROR: Cannot open dir %s\n", path);
   	  goto EXIT_CLEANUP;
         }
      } else {
         path = NULL;
         fd = -1;
     }
     printf5( "Generated and opened directory %s fd = %d\n", path, fd );      

     // Set up audit argument buffer
     if( ( rc = auditArg3( dataPtr, 
			   (dataPtr->successCase ? AUDIT_ARG_PATH : AUDIT_ARG_ERROR), 
			   (dataPtr->successCase ? strlen(path) : sizeof(int)),
			   (dataPtr->successCase ? path : (void*)&exp_errno),
			   //AUDIT_ARG_IMMEDIATE, sizeof( int ), &fd,
			   AUDIT_ARG_POINTER, 0,  &dir,
			   //AUDIT_ARG_POINTER, sizeof(struct dirent),  &dir,
			   AUDIT_ARG_IMMEDIATE, sizeof( unsigned int ), &count 
			   ) ) != 0 ) {      
         printf1( "Error setting up audit argument buffer\n" );
         goto EXIT_CLEANUP_OPENED;
         
     }

     // Do pre-system call work
     if ( (rc = preSysCall( dataPtr )) != 0 ) {
       printf1("ERROR: pre-syscall setup failed (%d)\n", rc);
       goto EXIT_CLEANUP;
     }
     
     // Execute system call - using syscall directly (readdir was seg faulting)
     dataPtr->laus_var_data.syscallData.result = syscall( __NR_readdir, fd, &dir, count );
 
     // Do post-system call work
     if ( (rc = postSysCall(  dataPtr, errno, -1, exp_errno  )) != 0 ) {
       printf1("ERROR: post-syscall setup failed (%d)\n", rc);
       goto EXIT_CLEANUP;
     }
     
    EXIT_CLEANUP_OPENED:
     if( dataPtr->successCase && close(fd) == -1 ) {
         printf1("Error closing directory %s: errno=%i\n", path, errno );
     }
    EXIT_CLEANUP:
      // Do cleanup work here - remove the temporary directory
   
     if( dataPtr->successCase  && (( rmdir( path )) == -1 )) {
         printf1( "Error removing directory %s: errno=%i\n", path, errno );
         }
     
    EXIT:
     if (path)
       free(path);
     printf5( "Returning from test\n" );
     return rc;
   }
   
#endif
