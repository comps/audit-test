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
   **  FILE   : test_write.c
   **
   **  PURPOSE: The test_write() function builds into the laus_test
   **           framework to verify that the Linux Audit System accurately
   **           logs both successful and erroneous execution of the
   **           "write" system call.
   **
   **           In the successful case, this function:
   **             1) Creates the temporary file
   **             2) Opens it in write-only mode
   **             3) Executes the "write" system call with a valid buffer
   **
   **           The successful case executes the expected conditions
   **           described by the "write" system call manpage.  That is,
   **           the write() function is called using a valid file descriptor
   **           to a writeable file and a valid buffer.
   **
   **            In the erroneous case, this function:
   **
   **            The erroneous case executes the faulty conditions
   **            described by the "EFAULT" errno under the "write" system
   **            system call manpage.  That is, the write() function is
   **            called using a NULL buffer.
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
   ** execute a write operation
   */
   int test_write(laus_data* dataPtr) {
     int rc = 0;
     int exp_errno = EFAULT;
     int fd = 0;
     char* writedata = "Bogus test file contents to write";
     int length = strlen(writedata);
     char* fileName = NULL;
     void* buf;
   
   
     // Set the syscall specific data
     dataPtr->laus_var_data.syscallData.code = __NR_write;
     // BUGBUG: Need to understand how to set up syscall parameters
   
     // Dynamically create temp file
     if ((rc = createTempFile(&fileName, S_IRWXU | S_IRWXG | S_IRWXO,
                          dataPtr->msg_euid, dataPtr->msg_egid)) == -1) {
       printf1("ERROR: Cannot create file %s\n", fileName);
       goto EXIT;
     }
     // Open file in write mode
     if ((fd = open(fileName, O_WRONLY)) == -1) {
      printf1("ERROR: Unable to open %s write only: errno=%i\n",
              fileName, errno);
       rc = fd;
       goto EXIT_CLEANUP;
     }
   
     if (dataPtr->successCase) {
       buf = (void *)malloc(length);
       strcpy(buf, writedata);
     } else {
       buf = NULL;
     }
   
     // Set up audit argument buffer
     if( ( rc = auditArg3( dataPtr,
			   AUDIT_ARG_PATH, strlen(fileName), fileName,
			   (dataPtr->successCase ? AUDIT_ARG_POINTER : AUDIT_ARG_NULL), 0, 
			   buf,
			   AUDIT_ARG_IMMEDIATE, sizeof(length), &length ) ) != 0 ) {
       printf1( "Error setting up audit argument buffer\n" );
       goto EXIT;
     }
   
     // Do pre-system call work
     if ( (rc = preSysCall( dataPtr )) != 0 ) {
       printf1("ERROR: pre-syscall setup failed (%d)\n", rc);
       goto EXIT_CLEANUP;
     }
   
     dataPtr->laus_var_data.syscallData.result = syscall( __NR_write, fd, buf, length );
   
     // Do post-system call work
     if ( (rc = postSysCall(  dataPtr, errno, -1, exp_errno  )) != 0 ) {
       printf1("ERROR: post-syscall setup failed (%d)\n", rc);
       goto EXIT_CLEANUP;
     }
   
   EXIT_CLEANUP: 
     // write cleanup
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
     if (fileName)
       free(fileName);
     return rc;
   }
