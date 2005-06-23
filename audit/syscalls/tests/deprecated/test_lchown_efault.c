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
   **  FILE       : test_lchown_efault.c
   **
   **  PURPOSE    : To test the lchown library call auditing.
   **
   **  DESCRIPTION: The test_lchown() function builds into the
   **  laus_test framework to verify that the Linux Audit System
   **  accurately logs both successful and erroneous execution of the
   **  "lchown" system call.
   **
   **  In the successful case, this function:
   **   1) Generate a file name and create a 777 test file owned by root
   **   2) Clear the audit trail
   **   3) Execute the "lchown" system call to change ownership to test user
   **   4) Tests the results of the system call against the
   **      expected successful return
   **
   **  The successful case creates a test file owned by root and with 777
   **  permssions and attempts to change the ownership of the file.
   **  As the file does exist, and the mode is 777, the lchown syscall
   **  should succeed.
   **  
   **  In the erroneous case, this function:
   **   1) Generate a file name and do not create the test file
   **   2) Clear the audit trail
   **   3) Execute the "lchown" system call on a file that does not exist
   **   4) Tests the results of the system call against the
   **      expected successful return
   **      
   **  The erroneous case attempts to change the owner of a file that does
   **  not exist.  This tests the system calls handling of the error
   **  described in the manpage and label with the EFAULT errno.
   **
   **  HISTORY    :
   **    06/03 Originated by Dustin Kirkland <k1rkland@us.ibm.com>
   **    03/04 Added exp_errno variable by D. Kent Soper <dksoper@us.ibm.com>
   **
   **********************************************************************/
   
   #include "syscalls.h"
   #include <sys/types.h>
   #include <unistd.h>
   
   int test_lchown_efault(laus_data* dataPtr) {
     
    
     int rc = 0;
     int exp_errno = EFAULT;
   
     char* fileName = NULL;
     char* linkName = NULL;
     int owner;
     int group;
     
     
     // Set the syscall-specific data
     printf5( "Setting laus_var_data.syscallData.code to %d\n", AUDIT_lchown );
     dataPtr->laus_var_data.syscallData.code = AUDIT_lchown;
     
     /**
      * Do as much setup work as possible right here
      */
     // Only root may lchown
     owner = dataPtr->msg_euid;
     group = dataPtr->msg_egid;
     dataPtr->msg_euid = 0;
     dataPtr->msg_egid = 0;
     dataPtr->msg_fsuid = 0;
     dataPtr->msg_fsgid = 0;
     // Generate unique filename
     if (dataPtr->successCase) {
       // Success case, so create file
       if ((rc = createTempFile(&fileName, S_IRWXU | S_IRWXG | S_IRWXO,
                                dataPtr->msg_euid, dataPtr->msg_egid)) == -1) {
         printf1("ERROR: Cannot create file %s\n", fileName);
         goto EXIT;
       }
       // Success case, so generate link name by creating temp file, then deleting it real quick
       if ((rc = createTempFile(&linkName, S_IRWXU | S_IRWXG | S_IRWXO,
                                dataPtr->msg_euid, dataPtr->msg_egid)) == -1) {
         printf1("ERROR: Cannot create file %s\n", fileName);
         goto EXIT;
       }
       // Delete link name to clear the space
       if ((rc = unlink(linkName)) != 0) {
         printf1("ERROR: Unable to remove file %s: errno=%i\n", linkName, errno);
         goto EXIT;
       }
       // Success case, so create a link
       if ((rc = link(fileName, linkName)) == -1) {
         printf1("ERROR: Cannot create link %s\n", linkName);
         goto EXIT_CLEANUP;
       }
     } else {
       linkName = NULL;
     }
   
     // Set up audit argument buffer
     if( ( rc = auditArg3( dataPtr,
                    dataPtr->successCase ? AUDIT_ARG_PATH : AUDIT_ARG_NULL,
                      dataPtr->successCase ? strlen( linkName ) : 0, linkName,
                    AUDIT_ARG_IMMEDIATE, sizeof( owner ), &owner,
                    AUDIT_ARG_IMMEDIATE, sizeof( group ), &group ) ) != 0 ) {
       printf1( "Error setting up audit argument buffer\n" );
       goto EXIT;
     }
   
     // Do pre-system call work
     if ( (rc = preSysCall( dataPtr )) != 0 ) {
       printf1("ERROR: pre-syscall setup failed (%d)\n", rc);
       goto EXIT_CLEANUP;
     }
   
     // Execute system call
     dataPtr->laus_var_data.syscallData.result = syscall( __NR_lchown, linkName, owner, group );
   
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
       if (( unlink(fileName)) != 0) {
         printf1("ERROR: Unable to remove file %s: errno=%i\n", fileName, errno);
         goto EXIT;
       }
       if (( unlink(linkName)) != 0) {
         printf1("ERROR: Unable to remove file %s: errno=%i\n", linkName, errno);
         goto EXIT;
       }
     }
   
    EXIT:
     if ( fileName )
       free( fileName );
     if ( linkName )
       free( linkName );
     printf5( "Returning from test\n" );
     return rc;
   }
