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
 **  FILE       : test_init_module.c
 **
 **  PURPOSE    : To test the init_module library call auditing.
 **
 **  DESCRIPTION: The test_init_module() function builds into the
 **  laus_test framework to verify that the Linux Audit System
 **  accurately logs both successful and erroneous execution of the
 **  "init_module" system call.
 **
 **  In the successful case, this function:
 **   1) Runs the ``insmod'' external program with a valid kernel
 **      module
 **   2) Verifies the success result
 **   3) Removes the newly loaded kernel module.
 **
 **  The successful case utilizes the insmod application from the
 **  modutils package to indirectly make the init_module call.  The
 **  process of setting up for this call is complicated and esoteric,
 **  and so we opted to use a previously existing utility that has the
 **  express purpose of making the init_module call.
 **  
 **  In the erroneous case, this function:
 **   1) Runs the ``insmod'' external program with an invalid
 **      (nonexistent) kernel module
 **   2) Verifies the error result.
 **      
 **  The erroneous case calls insmod with a bogus module path.
 **
 **  HISTORY    :
 **    07/03 Originated by Michael A. Halcrow <mike@halcrow.us>
 **    03/04 Added exp_errno variable by D. Kent Soper <dksoper@us.ibm.com>
 **
 **********************************************************************/

#include "includes.h"
#include "syscalls.h"

/* <linux/module.h> can't be included in userspace */
int delete_module(const char *);

int test_init_module(laus_data* dataPtr) {

	int rc = 0;
	int exp_errno = EEXIST;
	char dummy[] = { 0 };
	int size;
	char *module_name = "binfmt_misc";
	char test_module_path[PATH_MAX];
	char cmd[PATH_MAX];
	FILE *fPtr;

	// Set the syscall-specific data
	printf5( "Setting laus_var_data.syscallData.code to %d\n", AUDIT_init_module );
	dataPtr->laus_var_data.syscallData.code = AUDIT_init_module;

	//Do as much setup work as possible right here
	dataPtr->msg_euid = 0;
	dataPtr->msg_egid = 0;
	dataPtr->msg_fsuid = 0;
	dataPtr->msg_fsgid = 0;

	// Set up
	system( "echo /lib/modules/`uname -r`/kernel/fs/binfmt_misc.ko > /tmp/laus_module" );
	fPtr = fopen("/tmp/laus_module", "r");
  	fscanf( fPtr, "%s", test_module_path );
        fclose( fPtr );
	unlink("/tmp/laus_module");
		
	size = size_of_file(test_module_path);
	if (size < 0) {
		printf1( "Error getting module file size\n" );
		goto EXIT;
	}

	// Set up audit argument buffer
	if( ( rc = auditArg3( dataPtr,
					AUDIT_ARG_POINTER, 0, dummy,
					AUDIT_ARG_IMMEDIATE, sizeof(int), &size,
					AUDIT_ARG_STRING, 0, "" 
			    ) ) != 0 ) {
		printf1( "Error setting up audit argument buffer\n" );
		goto EXIT;
	}

	// Do pre-system call work
	if( ( rc = preSysCall( dataPtr ) ) != 0 ) {
		printf1("ERROR: pre-syscall setup failed (%d)\n", rc);
		goto EXIT_CLEANUP;
	}

	// Execute system call
	sprintf(cmd, "/sbin/insmod %s", test_module_path);

	// Insmod the module in both cases
	if ( (system( cmd )) != 0 ) {
		printf1("system() failed; cannot test init_module: errno=%i\n",
				errno);
		goto EXIT;
	}

	// In fail case, insmod same module again to cause EEXIST errno
	if ( !dataPtr->successCase ) {
		sleep(2);
		if ( (system( cmd )) != 0 ) {
			printf("system failed successfully with errno=%i\n", errno);
		} else {     
			printf1("system() failed; cannot test init_module for fail case\n");
			goto EXIT_CLEANUP;
		}
	}

	// We must make up these values; auditing will tell us if they really succeeded
	dataPtr->msg_pid = NO_PID_CHECK;
	if ( dataPtr->successCase ) {
		dataPtr->laus_var_data.syscallData.result = 0;
	} else {
		dataPtr->laus_var_data.syscallData.result = -1;
		dataPtr->laus_var_data.syscallData.resultErrno = exp_errno;
	}
	postSysCall( dataPtr, exp_errno, -1, exp_errno );

EXIT_CLEANUP:
	//Do cleanup work here
	// Clean up in both cases
	if( delete_module( module_name ) != 0 ) {
		printf1( "Error removing module [%s]\n", module_name );
	}

EXIT:
	printf5( "Returning from test\n" );

	return rc;
}
