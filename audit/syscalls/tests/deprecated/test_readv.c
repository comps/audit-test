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
    **  FILE       : test_readv.c
    **
    **  PURPOSE    : To test the readv library call auditing.
    **
    **  DESCRIPTION: The test_readv() function builds into the
    **  laus_test framework to verify that the Linux Audit System
    **  accurately logs both successful and erroneous execution of the
    **  "readv" system call.
    **
    **  In the successful case, this function:
    **   1) Creates a temporary file
    **   2) Opens the file for writing
    **   3) Writes some data to the file
    **   4) Closes the file
    **   5) Opens the file for reading
    **   6) Calls readv() on the file
    **   7) Verifies the success result.
    **
    **  The successful case performs a valid read operation on a valid
    **  open file.  All variables and data structures are set up for the
    **  success case according to the description in the man page for
    **  readv.
    **  
    **  In the erroneous case, this function:
    **   1) Calls readv() on a bad file descriptor
    **   2) Verifies the error result.
    **      
    **  The erroneous case calls readv() with fd=-1.  Since -1 is an
    **  invalid file descriptor, according to the man page for readv, we
    **  can expect an EBADF errno condition.
    **
    **  HISTORY    :
    **    06/03 Originated by Michael A. Halcrow <mike@halcrow.us>
    **
    **********************************************************************/
   
   #include "syscalls.h"
   #include <sys/uio.h>
   
   int test_readv(laus_data* dataPtr) {
     
     int rc = 0;
     int exp_errno = EBADF;
     char* fileName = NULL;
     
     int fd = -1;
     struct iovec vector;
     int count;
     
     const char* data = "data";
     char tempSpace[2];
   
     // Set the syscall-specific data
     printf5( "Setting laus_var_data.syscallData.code to %d\n", __NR_readv );
     dataPtr->laus_var_data.syscallData.code = __NR_readv;
     
     /**
      * Do as much setup work as possible right here
      */
     vector.iov_base = (__ptr_t)tempSpace;
     vector.iov_len = 1;
     count = 1;
     if( dataPtr->successCase ) {
       // Set up for success
       // Might include: dataPtr->msg_euid = 0; dataPtr->msg_egid = 0;
       // Create a new temporary file
       if( ( rc = createTempFile ( &fileName, S_IRWXU | S_IRWXG | S_IRWXO,
   				dataPtr->msg_euid, dataPtr->msg_egid ) ) == -1 ) {
         printf1( "ERROR: Cannot create file %s\n", fileName );
         goto EXIT;
       }
       if( ( fd = open( fileName, O_CREAT | O_WRONLY ) ) == -1 ) {
         printf1( "ERROR: Unable to open file %s for writing: errno=%i\n",
   	       fileName, errno );
         goto EXIT;
       }
       if( write( fd, data, strlen( data ) ) == -1 ) {
         printf1( "Error writing to file descriptor %d\n", fd );
         goto EXIT_CLEANUP;
       }
       if( close( fd ) == -1 ) {
         printf1( "Error closing file descriptor %d\n", fd );
         goto EXIT_CLEANUP;
       }
       if( ( fd = open( fileName, O_RDONLY ) ) == -1 ) {
         printf1( "ERROR: Unable to open file %s for reading: errno=%i\n",
   	       fileName, errno );
         goto EXIT_CLEANUP;
       }    
     } else {
       // Set up for error
       // We have a bad file descriptor
     }
   
     // Set up audit argument buffer
     if( ( rc = auditArg3( dataPtr,
			   (dataPtr->successCase ? AUDIT_ARG_PATH : AUDIT_ARG_ERROR), 
			   (dataPtr->successCase ? strlen(fileName) : sizeof( int )), 
			   (dataPtr->successCase ? fileName : (void*)&exp_errno ),
			   AUDIT_ARG_POINTER, 0, &vector,
			   //AUDIT_ARG_POINTER, sizeof( struct iovec ), &vector,
			   AUDIT_ARG_IMMEDIATE, sizeof( int ), &count ) ) != 0 ) {
	 printf1( "Error setting up audit argument buffer\n" );
	 goto EXIT_CLEANUP;
     }
     
     // Do pre-system call work
     if ( (rc = preSysCall( dataPtr )) != 0 ) {
       printf1("ERROR: pre-syscall setup failed (%d)\n", rc);
       goto EXIT_CLEANUP;
     }
   
     // Execute system call
     dataPtr->laus_var_data.syscallData.result = syscall( __NR_readv, fd, &vector, count );
   
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
       // Clean up from success case setup
       if( close( fd ) == -1 ) {
         printf1( "Error closing file with descriptor %d\n", fd );
         goto EXIT;
       }
       if( unlink( fileName ) == -1 ) {
         printf1( "Error unlinking file %s\n" );
         goto EXIT;
       }
     }
   
    EXIT:
     if (fileName)
       free(fileName);
     printf5( "Returning from test\n" );
     return rc;
   }
