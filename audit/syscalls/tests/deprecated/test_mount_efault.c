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
   **  FILE       : test_mount_efault.c
   **
   **  PURPOSE    : To test the mount library call auditing.
   **
   **  DESCRIPTION: The test_mount_efault() function builds into the
   **  laus_test framework to verify that the Linux Audit System
   **  accurately logs both successful and erroneous execution of the
   **  "mount" system call.
   **
   **  In the successful case, this function:
   **   1) Run test as root
   **   2) Create temporary target directory
   **   3) Use mount system call to mount the virtual proc filesystem
   **      at the temporary directory created in step 2
   **   4) Unmount the temporary directory
   **   5) Remove the temporary directory
   **
   **  The successful case mounts the virtual proc filesystem at the
   **  path specified in target.  Since the target directory is 
   **  created just before the mount and the test user is root we
   **  can expect mount() to successfully execute.
   **  
   **  In the erroneous case, this function:
   **   1) Run test as root
   **   2) Attemp mount system call to mount the virtual proc filesystem at NULL
   **      
   **  The erroneous case invokes an error by passing mount() a NULL pointer
   **  for the target directory of the mount.
   **
   **  HISTORY    :
   **    06/03 Originated by Kylene J. Smith <kylene@us.ibm.com>
   **    03/04 Added exp_errno variable by D. Kent Soper <dksoper@us.ibm.com>
   **
   **********************************************************************/
   
   #include "syscalls.h"
   #include <sys/mount.h>
   
   int test_mount_efault(laus_data* dataPtr) {
     
    
     int rc = 0;
     int exp_errno = EFAULT;
     char* source = "none";
     char* target = NULL;
     char* filesystem_type = "proc";
     unsigned long mountflags = 0;
     void* data = NULL;
     
     printf4( "Performing test_mount\n" );
     
     // Set the syscall-specific data
     printf5( "Setting laus_var_data.syscallData.code to %d\n", AUDIT_mount );
     dataPtr->laus_var_data.syscallData.code = AUDIT_mount;
   
     // must run as root
     dataPtr->msg_euid = 0;
     dataPtr->msg_egid = 0;
     dataPtr->msg_fsuid = 0;
     dataPtr->msg_fsgid = 0;
   
     if (dataPtr->successCase ) {
     // dynamically create test directory
         if (rc = (createTempDir(&target, S_IRWXU | S_IRWXG | S_IRWXO,
   			      dataPtr->msg_euid, dataPtr->msg_egid)) == -1) {
   	  printf1("ERROR: Cannot create dir %s\n", target);
   	  goto EXIT;
         }
         printf5("Genereated target directory %s\n", target);
     }
     
     // Set up audit argument buffer
     if( ( rc = auditArg5( dataPtr,
			   AUDIT_ARG_STRING, strlen( source ), source,
			   (dataPtr->successCase ? AUDIT_ARG_PATH : AUDIT_ARG_NULL), 
			   (dataPtr->successCase ? strlen( target ) : 0),
			   target,
			   AUDIT_ARG_STRING, strlen( filesystem_type ), filesystem_type,
			   AUDIT_ARG_IMMEDIATE, sizeof(unsigned long), &mountflags,
			   AUDIT_ARG_NULL, 0, data ) ) != 0 ) {
         printf1( "Error setting up audit argument buffer\n");
         goto EXIT_CLEANUP;
	 
     }
	 
	 // Do pre-system call work
	 if ( (rc = preSysCall( dataPtr )) != 0 ) {
       printf1("ERROR: pre-syscall setup failed (%d)\n", rc);
       goto EXIT_CLEANUP;
     }
   
     // Execute system call
     dataPtr->laus_var_data.syscallData.result = syscall ( AUDIT_mount, source, target, filesystem_type, mountflags, NULL );
   
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
         if ( umount( target ) == -1 )  {
   	  printf1( "Cannot umount %s\n", target );
         }
         
         if ( rmdir( target ) == -1 ) {
   	  printf1( "Error removing target directory %s: errno%i\n", 
   		   target, errno );
         }
     }   
    EXIT:
     if ( target )
       free(target);
     printf5( "Returning from test\n" );
     return rc;
   }
