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
 **  FILE       : test_delete_module.c
 **
 **  PURPOSE    : To test the delete_module library call auditing.
 **
 **  DESCRIPTION: The test_delete_module() function builds into the
 **  laus_test framework to verify that the Linux Audit System
 **  accurately logs both successful and erroneous execution of the
 **  "delete_module" system call.
 **
 **  In the successful case, this function:
 **   1) Executes create_module() on a unique, nonexistent module name
 **   2) Executes delete_module() on the module name.
 **   3) Tests the result of the syscall against the expected result
 **      for the successful case.
 **
 **  The successful case uses a unique, nonexistent module name to
 **  avoid a naming conflict with another module.  As the superuser,
 **  the delete_module() call should succeed, according to the
 **  information given on the man page for the syscall.
 **  
 **  In the erroneous case, this function:
 **   1) Sets the effective user ID to the test user
 **   2) Executes delete_module() on a unique, nonexistent module name
 **   3) Sets the effective user ID to the superuser
 **   4) Tests the result of the syscall against the expected result
 **      for the erroneous case.
 **      
 **  The erroneous case sets the effective user ID to the test user.
 **  According to the delete_module() man page, the syscall is only
 **  open to the superuser.  We can thus expect an EPERM error code as
 **  a result.
 **
 **  HISTORY    :
 **    06/03 Originated by Michael A. Halcrow <mike@halcrow.us>
 **    06/03 Furthered by Dustin Kirkland (k1rkland@us.ibm.com)
 **    03/04 Added exp_errno variable by D. Kent Soper <dksoper@us.ibm.com>
 **
 **********************************************************************/

#include "syscalls.h"

int test_delete_module(laus_data* dataPtr) {


	int rc = 0;
	int exp_errno = EPERM;
        int size;
	//char resolvedPath [ PATH_MAX ];    // not needed?
	//int pid, status;  		     // not needed?
	char *module_name = "binfmt_misc";
	char test_module_path[PATH_MAX];
	char cmd[PATH_MAX];
	FILE *fPtr;


	// Set the syscall-specific data
	dataPtr->laus_var_data.syscallData.code = AUDIT_delete_module;

	if( dataPtr->successCase ) {
		// only root can delete modules
		dataPtr->msg_euid = 0;
		dataPtr->msg_egid = 0;
		dataPtr->msg_fsuid = 0;
		dataPtr->msg_fsgid = 0;
	} 


	//load module
	system( "echo /lib/modules/`uname -r`/kernel/fs/binfmt_misc.ko > /tmp/laus_module" );
	fPtr = fopen("/tmp/laus_module", "r");
	fscanf( fPtr, "%s", &test_module_path );
	fclose( fPtr );
	unlink("/tmp/laus_module");

	size = size_of_file(test_module_path);
	if (size < 0) {
		printf1( "Error getting module file size\n" );
		goto EXIT;
	}

	// Execute system call
	sprintf(cmd, "/sbin/insmod %s", test_module_path);

	// Insmod the module in both cases
	if ( (system( cmd )) != 0 ) {
		printf1("system() failed; cannot test init_module: errno=%i\n",
				errno);
		goto EXIT;
	}


	// Set up audit argument buffer
	if ( dataPtr->successCase ) {
		if( ( rc = auditArg1( dataPtr,
						AUDIT_ARG_STRING, strlen( module_name ), 
						module_name ) ) != 0 ) {
			printf1( "Error setting up audit argument buffer\n" );
			goto EXIT_CLEANUP;
		}
	} else {
		if ( ( rc = auditArg1 ( dataPtr, 
						AUDIT_ARG_NULL, 0, NULL ) ) != 0 ) {
			printf1( "Error setting up audit argument buffer\n" );
			goto EXIT_CLEANUP;
		}
	}

	// Do pre-system call work
	if ( (rc = preSysCall( dataPtr )) != 0 ) {
		printf1("ERROR: pre-syscall setup failed (%d)\n", rc);
		goto EXIT_CLEANUP;
	}

	// Execute system call
	dataPtr->laus_var_data.syscallData.result = syscall( __NR_delete_module, module_name );

	// Do post-system call work
	if ( (rc = postSysCall(  dataPtr, errno, -1, exp_errno  )) != 0 ) {
		printf1("ERROR: post-syscall setup failed (%d)\n", rc);
	}


EXIT_CLEANUP:
	//Do cleanup work here
	if( ! dataPtr->successCase ) {
		// Clean up from success case setup
		if( delete_module( module_name ) == -1 ) {
			printf1( "Error removing module with command [rmmod %s]\n", module_name );
			goto EXIT;
		}
	}

EXIT:
	sleep(1);
	return rc;
}
