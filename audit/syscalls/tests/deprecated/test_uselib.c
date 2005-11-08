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
 **  FILE       : test_uselib.c
 **
 **  PURPOSE    : To test the uselib library call auditing.
 **
 **  DESCRIPTION: The test_uselib() function builds into the
 **  laus_test framework to verify that the Linux Audit System
 **  accurately logs both successful and erroneous execution of the
 **  "uselib" system call.
 **
 **  In the successful case, this function:
 **   1) Writes a dummy a.out shared library to disk
 **   2) Makes the uselib syscall against the dummy library.
 **
 **  The successful case creates a dummy shared library that has good
 **  magic numbers and file permissions in order to guarantee a
 **  success result from the uselib syscall.
 **  
 **  In the erroneous case, this function:
 **   1) Makes the uselib syscall against a nonexistent library.
 **      
 **  The erroneous case invokes the ENOENT errno by making the syscall
 **  with a library that does not exist.
 **
 **  Note that this test requires binfmt_aout to be linked into the
 **  kernel in order to succeed.
 **
 **  HISTORY    :
 **    09/03 Originated by Michael A. Halcrow <mike@halcrow.us>
 **    03/04 Added exp_errno variable by D. Kent Soper <dksoper@us.ibm.com>
 **
 **********************************************************************/

#include "syscalls.h"
#include <fcntl.h>
#include <linux/a.out.h>

#define LIBNAME	"/tmp/dummy.so"

unsigned char	text[4096];
struct exec	aout_hdr = {
	.a_info		= QMAGIC,
	.a_text		= sizeof(text),
	.a_data		= 0,
	.a_bss		= 0,
	.a_syms		= 0,
	.a_entry	= 0x01000000,
	.a_trsize	= 0,
	.a_drsize	= 0,
};

int writeLibrary() {

	int fd;

	if ((fd = open(LIBNAME, O_RDWR|O_TRUNC|O_CREAT, 0600)) < 0) {
		printf1( "open(%s): %m\n", LIBNAME);
		return 1;
	}

	write(fd, &aout_hdr, sizeof(aout_hdr));
	write(fd, text, sizeof(text));
	close(fd);

	//	uselib(LIBNAME);
	//	fprintf(stderr, "open(%s): %m\n", LIBNAME);
	//	unlink(LIBNAME);
	return 0;
}

int test_uselib( laus_data* dataPtr ) {
  
  int rc = 0;
  int exp_errno = ENOENT;
  char* library = LIBNAME;

#ifndef __IX86
  //we don't know how to make this succeed on any other platforms
  if ( dataPtr->successCase ) {
      rc = SKIP_TEST_CASE;
      goto EXIT;
  }
#endif

  // Set the syscall-specific data
  printf5( "Setting laus_var_data.syscallData.code to %d\n", __NR_uselib );
  dataPtr->laus_var_data.syscallData.code = __NR_uselib;
  
  /**
   * Do as much setup work as possible right here
   */
  if( dataPtr->successCase ) {
    // Set up for success
      dataPtr->msg_fsuid = dataPtr->msg_euid = 0;
      dataPtr->msg_fsgid = dataPtr->msg_egid = 0;
    if( chdir( cwd ) == -1 ) {
      printf1( "Error changing present working directory to [%s]\n", cwd );
      goto EXIT_CLEANUP;
    }
    if( rc = writeLibrary() ) {
      goto EXIT_CLEANUP;
    }
    system("if [ `grep binfmt_aout /proc/modules | wc -l` -eq 0 ];  then modprobe binfmt_aout; touch /tmp/laus_binfmt_aout; fi");
  } else {
    // Set up for error
    library = "laus_library_that_does_not_exist";
  }

  // Set up audit argument buffer
  if( rc = auditArg1( dataPtr,
		 (dataPtr->successCase ? AUDIT_ARG_PATH : AUDIT_ARG_STRING), 
		 strlen( library ), library ) != 0 ) {
    printf1( "Error setting up audit argument buffer\n" );
    goto EXIT;
  }

  // Do pre-system call work
  if ( (rc = preSysCall( dataPtr )) != 0 ) {
       printf1("ERROR: pre-syscall setup failed (%d)\n", rc);
       goto EXIT_CLEANUP;
  }
   

  // Execute system call
  dataPtr->laus_var_data.syscallData.result = syscall( __NR_uselib, library );

  // Do post-system call work
  if ( (rc = postSysCall( dataPtr, errno, -1, exp_errno )) != 0 ) {
      printf1("ERROR: post-syscall setup failed (%d)\n", rc);
       goto EXIT_CLEANUP;
  }

 EXIT_CLEANUP:
  /**
   * Do cleanup work here
   */
  if( dataPtr->successCase ) {
    // Clean up from success case setup
    unlink( library );
    system("if [ -e /tmp/laus_binfmt_aout ]; then rmmod binfmt_aout; rm -f /tmp/laus_binfmt_aout; fi");
  }

 EXIT:
  printf5( "Returning from test\n" );
  return rc;
}
