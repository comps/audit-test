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
   **  FILE   : test_truncate64_einval.c
   **
   **  PURPOSE: The test_truncate64() function builds into the laus_test
   **           framework to verify that the Linux Audit System accurately
   **           logs both successful and erroneous execution of the
   **           "truncate64" system call.
   **
   **           In the successful case, this function:
   **             1) Creates the temporary file
   **             2) Executes the "truncate64" system call with valid length
   **
   **           The successful case executes the expected conditions
   **           described by the "truncate64" system call manpage.  That is,
   **           the truncate64() function is called using a valid filename 
   **           and length.
   **
   **            In the erroneous case, this function:
   **             1) Creates the temporary file
   **             2) Executes the "truncate64" system call with an invalid
   **                length parameter
   **
   **            The erroneous case executes the faulty conditions
   **            described by the "EINVAL" error under the "truncate64" system
   **            system call manpage.  That is, the truncate64() function is
   **            called using a filename and a negative length.
   **
   **
   **  HISTORY:
   **    06/03 original version by Dustin Kirkland (k1rkland@us.ibm.com)
   **    07/03 64-bit version adapted by Michael A. Halcrow <mike@halcrow.us>
   **    03/04 Added exp_errno variable by D. Kent Soper <dksoper@us.ibm.com>
   **
   **********************************************************************/

#if !defined(__PPC) && !defined(__X86_64)   
   #include "syscalls.h"
   #include <unistd.h>
   
   /*
   ** execute a truncate64 operation
   */
   int test_truncate64_einval(laus_data* dataPtr) {
     
   
     int rc = 0;
     int exp_errno = EINVAL;
     long long length;
     size_t count = 80;
     char* fileName = NULL;
   
   
     // Set the syscall specific data
     dataPtr->laus_var_data.syscallData.code = AUDIT_truncate;
     // BUGBUG: Need to understand how to set up syscall parameters
   
     // dynamically create temp file name
     if ((rc = createTempFile(&fileName, S_IRWXU | S_IRWXG | S_IRWXO,
                          dataPtr->msg_euid, dataPtr->msg_egid)) == -1) {
       printf1("ERROR: Cannot create file %s\n", fileName);
       goto EXIT;
     }
   
     if (dataPtr->successCase) {
       // if testing success case, set valid length
       length = 1;
     } else {
	 // else set negative length
	 length = -1;
     }
   
     // Set up audit argument buffer
     //64 bit numbers logged as 2 longs, low and high, high is always 0 in our test case
     if( ( rc = auditArg2( dataPtr,
			   AUDIT_ARG_PATH, strlen(fileName), fileName,
			   AUDIT_ARG_IMMEDIATE, sizeof(length), &length
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
     dataPtr->laus_var_data.syscallData.result = syscall( __NR_truncate64, fileName, length );
   
     // Do post-system call work
     if ( (rc = postSysCall(  dataPtr, errno, -1, exp_errno  )) != 0 ) {
       printf1("ERROR: post-syscall setup failed (%d)\n", rc);
       goto EXIT_CLEANUP;
     }
   
   
   EXIT_CLEANUP:  
     // truncate64 cleanup
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

#endif   

