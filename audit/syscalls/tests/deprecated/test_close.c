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
   **  FILE   : test_close.c
   **
   **  PURPOSE: The test_close() function builds into the laus_test
   **           framework to verify that the Linux Audit System accurately
   **           logs both successful and erroneous execution of the
   **           "close" system call.
   **
   **           In the successful case, this function:
   **             1) Generates a unique file name
   **             2) Creates and opens the temporary file
   **             3) Clears the audit trail
   **             4) Executes the "close" system call
   **             5) Tests the results of the system call against the
   **                expected successful return
   **
   **           The successful case executes the expected conditions
   **           described by the "close" system call manpage.  That is,
   **           the close() function is called using a valid file
   **           descriptor, thereby freeing the file descriptor from
   **           refering to a file.
   **
   **            In the erroneous case, this function:
   **             1) DOES NOT create and open the temporary file
   **             2) Clears the audit trail
   **             3) Executes the "close" system call on a bad file
   **                descriptor
   **             5) Tests the results of the system call against the
   **                expected erroneous return
   **
   **            The erroneous case executes the faulty conditions
   **            described by the "EBADF" error under the "close" system
   **            system call manpage.  That is, the close() function is
   **            called using a bad file descriptor, and thus, the 
   **            operation fails.
   **
   **
   **  HISTORY:
   **    05/03 originated by Dan Jones (danjones@us.ibm.com)
   **    06/03 furthered by Dustin Kirkland (k1rkland@us.ibm.com)
   **
   **********************************************************************/
   
   #include "syscalls.h"
   #include <unistd.h>
   
   int test_close(laus_data* dataPtr) {
    
    
     int rc = 0;
     int fd = -1;
     int exp_errno = EBADF;
   
     char* fileName = NULL;
   
   
   
     // Set the syscall specific data
     dataPtr->laus_var_data.syscallData.code = __NR_close;
     // BUGBUG: Need to understand how to set up syscall parameters
   
     // Create and open the file if testing success case
     if (dataPtr->successCase) {
       // dynamically create temp file name
       if ((rc = createTempFile(&fileName, S_IRWXU | S_IRWXG | S_IRWXO,
                                dataPtr->msg_euid, dataPtr->msg_egid)) == -1) {
         printf1("ERROR: Cannot create file %s\n", fileName);
         goto EXIT;
       }
       // Open the file to be closed
       if ((fd = open(fileName, O_RDONLY)) == -1) {
         printf1("ERROR: Unable to open file %s: errno=%i\n",
              fileName, errno);
         goto EXIT_CLEANUP;
       }
     }
   
     // Set up audit argument buffer
     if( ( rc = auditArg1( dataPtr,
			   (dataPtr->successCase ? AUDIT_ARG_PATH : AUDIT_ARG_ERROR ), 
			   (dataPtr->successCase ? strlen( fileName ) : sizeof(int) ), 
			   (dataPtr->successCase ? fileName : (void*)&exp_errno ) ) ) != 0 ) {
       printf1( "Error setting up audit argument buffer\n" );
       goto EXIT;
     }
   
     // Do pre-system call work
     if ( (rc = preSysCall( dataPtr )) != 0 ) {
       printf1("ERROR: pre-syscall setup failed (%d)\n", rc);
       goto EXIT_CLEANUP;
     }
   
     // Execute system call
     dataPtr->laus_var_data.syscallData.result = syscall( __NR_close, fd );
   
     // Do post-system call work
     if ( (rc = postSysCall(  dataPtr, errno, -1, exp_errno  )) != 0 ) {
       printf1("ERROR: post-syscall setup failed (%d)\n", rc);
       goto EXIT_CLEANUP;
     }
   
   
   EXIT_CLEANUP:
   
     // close cleanup
     if (dataPtr->successCase) {
       if (( unlink(fileName)) != 0) {
         printf1("ERROR: Unable to remove file %s: errno=%i\n", fileName, errno);
         goto EXIT;
       }
     }
     
   EXIT:
     if (fileName)
       free( fileName );
     return rc;
   }
   
