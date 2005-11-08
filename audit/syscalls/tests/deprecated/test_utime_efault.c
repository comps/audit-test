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
   **  FILE   : test_utime_efault.c
   **
   **  PURPOSE: The test_utime_efault() function builds into the laus_test
   **           framework to verify that the Linux Audit System accurately
   **           logs both successful and erroneous execution of the
   **           "utime" system call.
   **
   **           In the successful case, this function:
   **             1) Creates the temporary file
   **             2) Creates the utime data structure
   **             3) Executes the "utime" system call
   **
   **           The successful case executes the expected conditions
   **           described by the "utime" system call manpage.  That is,
   **           the utime() function is called using a valid filename 
   **           and actually changes the access and modified timestamps
   **           on the file.
   **
   **            In the erroneous case, this function:
   **             1) Creates the utime data structure
   **             2) Executes the "utime" system call on NULL
   **
   **            The erroneous case executes the faulty conditions
   **            described by the "EFAULT" errno.
   **            That is, the utime() function is called on NULL.
   **
   **
   **  HISTORY:
   **    06/03 originated by Dustin Kirkland (k1rkland@us.ibm.com)
   **    03/04 Added exp_errno variable by D. Kent Soper <dksoper@us.ibm.com>
   **
   **********************************************************************/
   
   #include "syscalls.h"
   #include <sys/types.h>
   #include <utime.h>
   #include <sys/time.h>
   
   /*
   ** execute a utime operation
   */
   int test_utime_efault(laus_data* dataPtr) {
   
     struct utimbuf* utbuf;
     int rc = 0;
     int exp_errno = EFAULT;
     char* fileName = NULL;
   
     // Set the syscall specific data
     dataPtr->laus_var_data.syscallData.code = AUDIT_utimes;
  
     // Create the file if testing success case
     if (dataPtr->successCase) {
       if ((rc = createTempFile(&fileName, S_IRWXU | S_IRWXG | S_IRWXO,
                         dataPtr->msg_euid, dataPtr->msg_egid)) == -1) {
         printf1("ERROR: Cannot create file %s\n", fileName);
         goto EXIT;
       }
     } else {
       fileName = NULL;
     }
 
     // utime setup
     utbuf = (struct utimbuf*)malloc(sizeof(struct utimbuf));
     utbuf->actime = time(NULL);
     utbuf->modtime = time(NULL);
   
     // Set up audit argument buffer
     if( ( rc = auditArg2( dataPtr,
			   (dataPtr->successCase ? AUDIT_ARG_PATH : AUDIT_ARG_NULL), 
			   (dataPtr->successCase ? strlen(fileName) : 0), 
			   fileName,
			   AUDIT_ARG_POINTER, sizeof(struct utimbuf), utbuf 
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
     dataPtr->laus_var_data.syscallData.result = syscall( __NR_utime, fileName, utbuf );
   
     // Do post-system call work
     if ( (rc = postSysCall(  dataPtr, errno, -1, exp_errno  )) != 0 ) {
       printf1("ERROR: post-syscall setup failed (%d)\n", rc);
       goto EXIT_CLEANUP;
     }
   
   EXIT_CLEANUP:  
     // utime cleanup
     if( dataPtr->successCase ) {
       if ((rc = unlink(fileName)) != 0) {
         printf1("ERROR: Unable to remove file %s: errno=%i\n", 
     	      fileName, errno);
         goto EXIT;
       }
     }
     
   EXIT:
     free(utbuf);
     if (fileName)
       free(fileName);
     return rc;
   }
   
