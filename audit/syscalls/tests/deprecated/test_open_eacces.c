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
   **  FILE   : test_open_eacces.c
   **
   **  PURPOSE: The test_open_eacces() function builds into the laus_test
   **           framework to verify that the Linux Audit System accurately
   **           logs both successful and erroneous execution of the
   **           "open" system call.
   **
   **           In the successful case, this function:
   **             1) Creates the temporary file
   **             2) Executes the "open" system call
   **             3) Tests the results of the system call against the
   **                expected successful return
   **
   **           The successful case executes the expected conditions
   **           described by the "open" system call manpage.  That is,
   **           the open() function is called using an existing, readable
   **           filename, according to valid flags, and returns a file 
   **           descriptor to be used for subsequent input/output
   **           operations.
   **
   **            In the erroneous case, this function:
   **             1) Executes the "open" system call on a file that the
   **                user does not have permission to read
   **             2) Tests the results of the system call against the 
   **                expected erroneous return 
   **
   **            The erroneous case executes the faulty conditions
   **            described by the "EACCES" error under the "open" system
   **            system call manpage.  That is, the open() function is
   **            called using a file that the user cannot read.
   **
   **           
   **
   **
   **  HISTORY:
   **    05/03 originated by Dan Jones (danjones@us.ibm.com)
   **    06/03 furthered by Dustin Kirkland (k1rkland@us.ibm.com)
   **    03/04 Added exp_errno variable by D. Kent Soper <dksoper@us.ibm.com>
   **
   **********************************************************************/
   
   #include "syscalls.h"
   #include <sys/types.h>
   #include <sys/stat.h>
   #include <fcntl.h>
   
   int test_open_eacces(laus_data* dataPtr) {
    
    
     int rc = 0;
     int exp_errno = EACCES;
     int fd = 0;
     int flags = O_RDONLY;
     int mode = S_IRWXU;       // mode is ignored in this test case
     char* fileName = NULL;
   
   
   
     // Set the syscall specific data
     dataPtr->laus_var_data.syscallData.code = AUDIT_open;
   
     // Create the file if testing success case
     if (dataPtr->successCase) {
       if ((rc = createTempFile(&fileName, S_IRWXU | S_IRWXG | S_IRWXO,
   			 dataPtr->msg_euid, dataPtr->msg_egid)) == -1) {
         printf1("ERROR: Cannot create file %s\n", fileName);
         goto EXIT;
       }
     } else {
       if ((rc = createTempFile(&fileName, S_IRWXU,
                         dataPtr->msg_euid, dataPtr->msg_egid)) == -1) {
         printf1("ERROR: Cannot create file %s\n", fileName);
         goto EXIT;
       }
       dataPtr->msg_euid = dataPtr->msg_ruid = dataPtr->msg_fsuid = helper_uid;
     }
   
     // Set up audit argument buffer
     if( ( rc = auditArg3( dataPtr,
                         AUDIT_ARG_PATH, 
                         strlen(fileName), 
                         fileName,
                         AUDIT_ARG_IMMEDIATE, sizeof(flags), &flags,
                         AUDIT_ARG_IMMEDIATE, sizeof(mode), &mode
                       ) ) != 0 ) {
       printf1( "Error setting up audit argument buffer\n" );
       goto EXIT;
     }
   
     // Do pre-system call work
     if ( (rc = preSysCall( dataPtr )) != 0 ) {
       printf1("ERROR: pre-syscall setup failed (%d)\n", rc);
       goto EXIT_CLEANUP;
     }
   
     // Execute system call
     fd = dataPtr->laus_var_data.syscallData.result = syscall( __NR_open, fileName, flags, mode );
   
     // Do post-system call work
     if ( (rc = postSysCall(  dataPtr, errno, -1, exp_errno  )) != 0 ) {
       printf1("ERROR: post-syscall setup failed (%d)\n", rc);
       goto EXIT_CLEANUP;
     }
   
   
   
   EXIT_CLEANUP:    
     // open cleanup
     if (dataPtr->successCase) {
       // close file is successfully opened
       if ((rc = close(fd)) != 0) {
         printf1("ERROR: Unable to close file %s: errno=%i\n", fileName, errno);
         goto EXIT;
       }
     }
     if ((rc = unlink(fileName)) != 0) {
       printf1("ERROR: Unable to remove file %s: errno=%i\n", fileName, errno);
       goto EXIT;
     }
     
   EXIT:
     if (fileName)
       free(fileName);
     return rc;
   }
   
