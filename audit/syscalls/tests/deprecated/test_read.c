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
   **  FILE   : test_read.c
   **
   **  PURPOSE: The test_read() function builds into the laus_test
   **           framework to verify that the Linux Audit System accurately
   **           logs both successful and erroneous execution of the
   **           "read" system call.
   **
   **           In the successful case, this function:
   **             1) Creates the temporary file
   **             2) Opens it in read-only mode
   **             3) Mallocs memory for the read buffer
   **             4) Executes the "read" system call
   **
   **           The successful case executes the expected conditions
   **           described by the "read" system call manpage.  That is,
   **           the read() function is called using a valid file descriptor
   **           to a readable file, and a valid buffer.
   **
   **            In the erroneous case, this function:
   **             1) Creates the temporary file
   **             2) Opens it in read-only mode
   **             3) Sets the buffer to NULL
   **             4) Executes the "read" system call
   **
   **            The erroneous case executes the faulty conditions
   **            described by the "EFAULT" error under the "read" system
   **            system call manpage.  That is, the read() function is
   **            called using a bad buffer.
   **
   **
   **  HISTORY:
   **    05/03 originated by Dan Jones (danjones@us.ibm.com)
   **    06/03 furthered by Dustin Kirkland (k1rkland@us.ibm.com)
   **    03/04 Added exp_errno variable by D. Kent Soper <dksoper@us.ibm.com>
   **
   **********************************************************************/
   
   #include "syscalls.h"
   #include <unistd.h>
   
   /*
   ** execute a read operation
   */
   int test_read(laus_data* dataPtr) {
     
   
     int rc = 0;
     int exp_errno = EFAULT;
     int fd = 0;
     size_t count = 80;
     void* buff = NULL;
     char* fileName = NULL;
   
   
   
     // Set the syscall specific data
     dataPtr->laus_var_data.syscallData.code = __NR_read;
     // BUGBUG: Need to understand how to set up syscall parameters
   
     // read setup
     if ((rc = createTempFile(&fileName, S_IRWXU | S_IRWXG | S_IRWXO,
                          dataPtr->msg_euid, dataPtr->msg_egid)) == -1) {
       printf1("ERROR: Cannot create file %s\n", fileName);
       goto EXIT;
     }
     if (dataPtr->successCase ) {
       // success case, so malloc some space for the buffer
       buff = (void*)malloc((int)count);
     } else {
       // error case, so buffer is NULL
       buff = NULL;
     }
   
     // open file in read mode
     if ((fd = open(fileName, O_RDONLY)) == -1) {
       printf1("ERROR: Unable to open file %s read only: errno=%i\n",
            fileName, errno);
       rc = fd;
       goto EXIT;
     }
   
     // Set up audit argument buffer
     if( ( rc = auditArg3( dataPtr,
			   AUDIT_ARG_PATH, strlen(fileName), fileName,
			   (dataPtr->successCase ? AUDIT_ARG_POINTER : AUDIT_ARG_NULL), 0, buff,
			   AUDIT_ARG_IMMEDIATE, sizeof(count), &count ) ) != 0 ) {
       printf1( "Error setting up audit argument buffer\n" );
       goto EXIT;
     }
   
     // Do pre-system call work
     if ( (rc = preSysCall( dataPtr )) != 0 ) {
       printf1("ERROR: pre-syscall setup failed (%d)\n", rc);
       goto EXIT_CLEANUP;
     }
   
     // Execute system call
     dataPtr->laus_var_data.syscallData.result = syscall( __NR_read, fd, buff, count );
   
     // Do post-system call work
     if ( (rc = postSysCall(  dataPtr, errno, -1, exp_errno  )) != 0 ) {
       printf1("ERROR: post-syscall setup failed (%d)\n", rc);
       goto EXIT_CLEANUP;
     }
   
   
   EXIT_CLEANUP:  
     // read cleanup
     if ((rc = close(fd)) != 0) {
       printf1("ERROR: Unable to close file %s: errno=%i\n", 
   	   fileName, errno);
       goto EXIT;
     }
     if ((rc = unlink(fileName)) != 0) {
       printf1("ERROR: Unable to remove file %s: errno=%i\n", 
   	   fileName, errno);
       goto EXIT;
     }
     
   EXIT:
   
     if (buff) {
       free(buff);
     }
     if (fileName)
       free(fileName);
     return rc;
   }
   
