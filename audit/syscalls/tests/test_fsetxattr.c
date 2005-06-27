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
 **  FILE       : test_fsetxattr.c
 **
 **  PURPOSE    : To test the fsetxattr library call auditing.
 **
 **  DESCRIPTION: The test_fsetxattr() function builds into the
 **  laus_test framework to verify that the Linux Audit System
 **  accurately logs both successful and erroneous execution of the
 **  "fsetxattr" system call.
 **
 **  In the successful case, this function:
 **   1) Creates a temporary file
 **   2) Opens the temporary file
 **   3) Calls fsetxattr on the opened file with
 **      name="user.mime_type" value=XATTR_TEST_VALUE flags=XATTR_CREATE
 **   4) Verifies the success result.
 **
 **  The successful case passes a valid file descriptor, name, value,
 **  size, and flag to the  fsetxattr call, thus satisfying the
 **  conditions as given in the man page for fsetxattr for a success
 **  result.
 **  
 **  In the erroneous case, this function:
 **   1) Creates a temporary file
 **   2) Opens the temporary file
 **   3) Calls fsetxattr on the opened file with
 **      name="user.mime_type" value=XATTR_TEST_VALUE flags=XATTR_REPLACE
 **   4) Verifies the erroneous result.
 **      
 **  The erroneous case causes an ENOATTR as detailed in the man page by 
 **  trying to replace and attribute which has not been set.
 **
 **  HISTORY    :
 **    07/03 Originated by Michael A. Halcrow <mike@halcrow.us>
 **    07/03 Furthered by Kylene J. Smith <kylene@us.ibm.com>
 **    03/04 Added exp_errno variable by D. Kent Soper <dksoper@us.ibm.com>
 **
 **********************************************************************/

#include "includes.h"
#include "syscalls.h"
#include <attr/xattr.h>

int test_fsetxattr(laus_data* dataPtr) {

	int rc = 0;
	int exp_errno = ENOATTR;
	char* path = NULL;

	int filedes;
	char* name = "user.mime_type";
	char value[ sizeof( XATTR_TEST_VALUE ) ];
	size_t size;
	int flags;

	// Set the syscall-specific data
	printf5( "Setting laus_var_data.syscallData.code to %d\n", AUDIT_fsetxattr );
	dataPtr->laus_var_data.syscallData.code = AUDIT_fsetxattr;

	/**
	 * Do as much setup work as possible right here
	 */
	bzero( value, sizeof( XATTR_TEST_VALUE ) );
	size = sizeof( XATTR_TEST_VALUE );

	// Create the target file
	if( ( rc = createTempFile( &path, ( S_IRWXU | S_IRWXG | S_IRWXO ),
					dataPtr->msg_euid, dataPtr->msg_egid ) ) == -1 ) {
		printf1( "ERROR: Cannot create file %s\n", path );
		goto EXIT;
	}
	// Open the target file
	if( ( filedes = rc = open( path, O_RDWR ) ) == -1 ) {
		printf1( "Error opening file [%s] for read/write access: errno=%i\n", path, errno );
		goto EXIT_CLEANUP_UNLINK;
	}

	if( dataPtr->successCase ) {     // Set up for success
		flags = XATTR_CREATE;
		strcpy( value, XATTR_TEST_VALUE );
	} else {        // Set up for error
		flags = XATTR_REPLACE;
	}

	// Set up audit argument buffer
	if( ( rc = auditArg5( dataPtr,
					AUDIT_ARG_PATH, strlen( path ), path,
					AUDIT_ARG_STRING, strlen( name ), name,
					AUDIT_ARG_POINTER, size, value,
					AUDIT_ARG_IMMEDIATE, sizeof( size_t ), &size,
					AUDIT_ARG_IMMEDIATE, sizeof( int ), &flags ) ) != 0 ) {
		printf1( "Error setting up audit argument buffer\n" );
		goto EXIT_CLEANUP_CLOSE;
	}

	// Do pre-system call work
	preSysCall( dataPtr );

	// Execute system call
	dataPtr->laus_var_data.syscallData.result = syscall( __NR_fsetxattr, filedes, name, value, size, flags );

	// Do post-system call work
	postSysCall( dataPtr, errno, -1, exp_errno );

EXIT_CLEANUP_CLOSE:
	if( (  close( filedes ) ) == -1 ) {
		printf1( "Error closing filehandle %d\n", filedes );
		goto EXIT_CLEANUP_UNLINK;
	}    

EXIT_CLEANUP_UNLINK:
	// Clean up from success case setup
	if( (  unlink( path ) ) == -1 ) {
		printf1( "Error unlinking file %s\n", path );
		goto EXIT;
	}

EXIT:
	if ( path )
		free( path );
	printf5( "Returning from test\n" );
	return rc;
}

