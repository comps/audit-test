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
   **  FILE       : test_umount.c
   **
   **  PURPOSE    : To test the umount library call auditing.
   **
   **  DESCRIPTION: The test_umount() function builds into the
   **  laus_test framework to verify that the Linux Audit System
   **  accurately logs both successful and erroneous execution of the
   **  "umount" system call.
   **
   **  In the successful case, this function:
   **   1) Run test as root
   **   2) Create temporary target directory
   **   3) To setup mount system call to mount the virtual proc filesystem
   **      at the temporary directory created in step 2
   **   4) Test: umount on temporary directory
   **   5) Remove the temporary directory
   **
   **  The successful case mounts the virtual proc filesystem at the
   **  path specified in target.  Since the target directory is 
   **  created just before the mount and the test user is root we
   **  can expect mount() to successfully execute.
   **  
   **  In the erroneous case, this function:
   **   1) Run test as non-root
   **   2) Attemp to umount good, mounted test directory
   **      
   **  The erroneous case invokes an error by attempting to call umount()
   **  as a non-root user which causes an EPERM.
   **
   **  HISTORY    :
   **    06/03 Originated by Kylene J. Smith <kylene@us.ibm.com>
   **    03/04 Added exp_errno variable by D. Kent Soper <dksoper@us.ibm.com>
   **
   **********************************************************************/
#ifndef __X86_64

   #include "includes.h"
   #include "syscalls.h"
   #include <sys/mount.h>
   
   int test_umount(laus_data* dataPtr) {
     
    
     int rc = 0;
     int exp_errno = EPERM;
     char* target = NULL;
     int dummy = 0;
     
     printf4( "Performing test_umount\n" );
     
     // Set the syscall-specific data
     printf5( "Setting laus_var_data.syscallData.code to %d\n", AUDIT_umount );
     dataPtr->laus_var_data.syscallData.code = AUDIT_umount;
   
     // dynamically create test directory
     if (rc = (createTempDir(&target, S_IRWXU | S_IRWXG | S_IRWXO,
		dataPtr->msg_euid, dataPtr->msg_egid)) == -1) {
	printf1("ERROR: Cannot create dir %s\n", target);
	goto EXIT;
     }
     printf5("Genereated target directory %s\n", target);
         
     if (rc = (mount( "none", target, "proc", 0, NULL) ) == -1) {
	printf1("ERROR: Cannot mount dir %s\n", target);
	goto EXIT_CLEANUP;
     }
     printf5("Mounted source directory \"none\" to "
	     "target directory %s\n", target);

     if (dataPtr->successCase ) {
	// must run as root
	dataPtr->msg_euid = 0;
	dataPtr->msg_egid = 0;
	dataPtr->msg_fsuid = 0;
	dataPtr->msg_fsgid = 0;
     }

     // Set up audit argument buffer
     if( ( rc = auditArg2( dataPtr,
   		    AUDIT_ARG_PATH, strlen( target ), target,
		    AUDIT_ARG_IMMEDIATE, sizeof( int), &dummy ) ) != 0 ) {
         printf1( "Error setting up audit argument buffer\n");
         goto EXIT_CLEANUP;
     }

     // Do pre-system call work
     if ( (rc = preSysCall( dataPtr )) != 0 ) {
       printf1("ERROR: pre-syscall setup failed (%d)\n", rc);
       goto EXIT_CLEANUP;
     }
   
     // Execute system call
     dataPtr->laus_var_data.syscallData.result = syscall ( __NR_umount, target );
   
     // Do post-system call work
     if ( (rc = postSysCall(  dataPtr, errno, -1, exp_errno  )) != 0 ) {
       printf1("ERROR: post-syscall setup failed (%d)\n", rc);
       goto EXIT_CLEANUP;
     }
   
    EXIT_CLEANUP:
     /**
      * Do cleanup work here
      */
     if ( ! dataPtr->successCase ) {
	if ( umount( target ) == -1 ) {
		printf1( "Error umounting target directory [%s] for deletion."
			 "  Errno: %i\n", target, errno );
	}
     }
     if (  rmdir( target ) == -1 ) {
	printf1( "Error removing target directory %s: errno%i\n", target, errno );
     }

    EXIT:
     if (target) {
	free(target);
     }
     printf5( "Returning from test\n" );
     return rc;
   }
#endif
