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
   **  FILE   : test_mknod.c
   **
   **  PURPOSE: The test_mknod() function builds into the laus_test
   **           framework to verify that the Linux Audit System accurately
   **           logs both successful and erroneous execution of the
   **           "mknod" system call.
   **
   **           In the successful case, this function:
   **             1) Generates a unique file name
   **             2) Clears the audit trail
   **             3) Executes the "mknod" system call
   **             4) Tests the results of the system call against the
   **                expected successful return
   **
   **           The successful case executes the expected conditions
   **           described by the "mknod" system call manpage.  That is,
   **           the mknod() function is called using a unique
   **           filename, according to a valid mode, and thus creates
   **           the node and returns 0.
   **
   **            The erroneous case executes the faulty conditions
   **            described by the "EACCES" error under the "mknod" system
   **            system call manpage.  That is, the mknod() function is
   **            called using the /root directory as a non-root user, thus the
   **            operation fails.
   **
   **           
   **
   **
   **  HISTORY:
   **    06/03 originated by Dustin Kirkland (k1rkland@us.ibm.com)
   **    03/04 Added exp_errno variable by D. Kent Soper <dksoper@us.ibm.com>
   **
   **********************************************************************/
   
   #include "syscalls.h"
   #include <sys/types.h>
   #include <sys/stat.h>
   #include <unistd.h>
   
   int test_mknod(laus_data* dataPtr) {
    
    
     int rc = 0;
     int exp_errno = EACCES;
     mode_t mode = S_IRWXO | S_IFIFO;
     dev_t dev = 0;
     char* fileName = NULL;
   
   
   
     // Set the syscall specific data
     dataPtr->laus_var_data.syscallData.code = AUDIT_mknod;
   
     if (dataPtr->successCase) {
       // dynamically create temp file, and delete it real quick
       if ((rc = createTempFile(&fileName, S_IRWXU | S_IRWXG | S_IRWXO,
                                dataPtr->msg_euid, dataPtr->msg_egid)) == -1) {
         printf1("ERROR: Cannot create file %s\n", fileName);
         goto EXIT;
       }
       if ((rc = unlink(fileName)) != 0) {
         printf1("ERROR: Unable to remove file %s: errno=%i\n", fileName, errno);
         goto EXIT;
       }
     } else {
       fileName = mysprintf("/root/tmp");
       unlink(fileName);
       dataPtr->msg_euid = dataPtr->msg_ruid = dataPtr->msg_fsuid = helper_uid;
     }
   
     // Set up audit argument buffer

     //hard code the size of dev because dev_t in the kernel is different
     if( ( rc = auditArg3( dataPtr,
                    dataPtr->successCase ? AUDIT_ARG_PATH : AUDIT_ARG_STRING,
                    strlen( fileName ), fileName,
                    AUDIT_ARG_IMMEDIATE, sizeof(mode), &mode,
                    AUDIT_ARG_IMMEDIATE, 4, &dev ) ) != 0 ) {
       printf1( "Error setting up audit argument buffer\n" );
       goto EXIT;
     }
   
     // Do pre-system call work
     if ( (rc = preSysCall( dataPtr )) != 0 ) {
       printf1("ERROR: pre-syscall setup failed (%d)\n", rc);
       goto EXIT_CLEANUP;
     }
   
     // Execute system call
     dataPtr->laus_var_data.syscallData.result = syscall( __NR_mknod, fileName, mode, dev );
   
     // Do post-system call work
     if ( (rc = postSysCall(  dataPtr, errno, -1, exp_errno  )) != 0 ) {
       printf1("ERROR: post-syscall setup failed (%d)\n", rc);
       goto EXIT_CLEANUP;
     }
   
   
   EXIT_CLEANUP:
     // mknod cleanup
     if (dataPtr->successCase) {
       if (( unlink(fileName)) != 0) {
         printf1("ERROR: Unable to remove file %s: errno=%i\n", fileName, errno);
         goto EXIT;
       }
     }
     
   EXIT:
     if ( fileName )
       free( fileName );
     return rc;
   }
   
