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
 **  FILE       : test_chroot.c
 **
 **  PURPOSE    : To test the chroot library call auditing.
 **
 **  DESCRIPTION: The test_chroot() function builds into the
 **  laus_test framework to verify that the Linux Audit System
 **  accurately logs both successful and erroneous execution of the
 **  "chroot" system call.
 **
 **  In the successful case, this function:
 **   1) Sets the euid to the superuser
 **   2) Creates a new temporary directory
 **   3) Forks; child process calls chroot and exits
 **   4) Parent process waits for child process to terminate
 **   5) Sets the euid to the test user
 **   6) Verifies the success result.
 **
 **  The successful case sets up the environment to assure that chroot
 **  returns a success result.  It sets the euid to the superuser,
 **  creates a temporary directory, and finally forks in order to
 **  preserve the validity of the parent test environment.
 **  
 **  In the erroneous case, this function:
 **   1) Makes the chroot system call
 **   2) Verifies the error result.
 **      
 **  The erroneous case calls chroot as the test user.  Since only the
 **  superuser may change the root directory, we will have an EPERM
 **  error.
 **
 **  HISTORY    :
 **    06/03 Originated by Michael A. Halcrow <mike@halcrow.us>
 **    03/04 Added exp_errno variable by D. Kent Soper <dksoper@us.ibm.com>
 **    05/04 Updates to suppress compiler warnings by Kimberly D. Simon <kdsimon@us.ibm.com>
 **
 **********************************************************************/
   
#include "includes.h"
#include "syscalls.h"
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/wait.h>

typedef struct errnoAndReturnValue_s {
  int returnValue;
  int savedErrno;
} errnoAndReturnValue_t;
   
int test_chroot(laus_data* dataPtr) {
     
  int rc = 0;
  int exp_errno = EPERM;
  int mode;
  //int pid;     // variables not needed?
  //key_t shmkey;
  //int shmsize;
  //int shmflg;
  int shmid;

  errnoAndReturnValue_t* earv;
  int savedErrno;
   
  char* path = NULL;
  char* tempPath = "/tmp";
     
  // Set the syscall-specific data
  printf5( "Setting laus_var_data.syscallData.code to %d\n", AUDIT_chroot );
  dataPtr->laus_var_data.syscallData.code = AUDIT_chroot;
     
  // Do as much setup work as possible right here

  // Initialize IPC
  // Shared memory
  if( ( shmid = shmget( IPC_PRIVATE, sizeof( errnoAndReturnValue_t ), 
			IPC_CREAT ) ) == -1 ) {
    printf1( "Error getting shared memory: errno=%i\n", errno );
    goto EXIT; // TODO: Explicitely account for the fact that the semaphore has been created at this point
  }
  if( dataPtr->successCase ) {
   // Set up for success
    dataPtr->msg_euid = 0;
    dataPtr->msg_egid = 0;
    dataPtr->msg_fsuid = 0;
    dataPtr->msg_fsgid = 0;
    mode = S_IRWXU | S_IRWXG | S_IRWXO;
    if( ( rc = createTempFile( &path, mode,
   			       dataPtr->msg_euid, dataPtr->msg_egid ) ) == -1 ) {
      printf1("ERROR: Cannot create file %s\n", path);
      goto EXIT;
    }
    if( ( rc = unlink( path ) ) != 0 ) {
      printf1("ERROR: Unable to remove file %s: errno=%i\n", path, errno);
      goto EXIT;
    }
    printf5( "Generated directory name %s\n", path );
    if( mkdir( path, mode ) == -1 ) {
      printf1( "Cannot create directory %s\n", path );
      goto EXIT;
    }
  } else {
    // Set up for error
    path = tempPath; // Just so it points somewhere; we want an EPERM errno though
  }
   
  // Set up audit argument buffer
  if (( rc = auditArg1( dataPtr,
		      AUDIT_ARG_PATH,
		      strlen(path), 
		      path ) != 0 )) {
    printf1( "Error setting up audit argument buffer\n" );
      goto EXIT;
  }

  // Do pre-system call work
  if ( (rc = preSysCall( dataPtr )) != 0 ) {
    printf1("ERROR: pre-syscall setup failed (%d)\n", rc);
    goto EXIT_CLEANUP;
  }
   
  // Execute system call
  if( dataPtr->successCase ) {
    int pid;
    pid = fork();
    if( pid == 0 ) {
      errnoAndReturnValue_t* childEarv;
      if( ( (long)( childEarv = shmat( shmid, NULL, 0 ) ) ) == -1 ) {
      printf1( "Error attaching to shared memory segment with id %d: errno=%d\n", shmid, errno );
      // TODO: Something a bit more drastic should happen at this point
      _exit( 0 );
      }
      childEarv->returnValue = syscall( __NR_chroot, path );
      childEarv->savedErrno = errno;
      if( shmdt( childEarv ) == -1 ) {
	printf1( "Error detaching from shared memory segment at address 0x%p: errno=%i\n",childEarv, errno );
	_exit( 0 );
      }
      _exit( 0 );
    } else {
      dataPtr->msg_pid = pid;
      if( waitpid( pid, NULL, 0 ) == -1 ) {
	printf1( "Error waiting on pid %d: errno=%i\n", pid, errno );
	goto EXIT_CLEANUP;
      }
      if( ( (long)( earv = shmat( shmid, NULL, 0 ) ) ) == -1 ) {
	printf1( "Error attaching to shared memory segment with id %d: errno=%i\n", shmid, errno );
	goto EXIT_CLEANUP;
      }
      dataPtr->laus_var_data.syscallData.result = earv->returnValue;
      savedErrno = earv->savedErrno;
      if( shmdt( earv ) == -1 ) {
	printf1( "Error detaching from shared memory segment at address 0x%p: errno=%i\n", earv, errno );
	goto EXIT_CLEANUP;
      }      
    }

  } else {
    dataPtr->laus_var_data.syscallData.result = syscall( __NR_chroot, path );
    savedErrno = errno;
  }
   
  // Do post-system call work
  if ( (rc = postSysCall(  dataPtr, savedErrno, -1, exp_errno  )) != 0 ) {
    printf1("ERROR: post-syscall setup failed (%d)\n", rc);
    goto EXIT_CLEANUP;
  }
   
 EXIT_CLEANUP:
  /**
   * Do cleanup work here
   */
  if( dataPtr->successCase ) {
    // Clean up from success case setup
    // Delete the temporary directory
    if( rmdir( path ) == -1 ) {
      printf1( "Cannot remove directory %s: errno=%i\n", path, errno );
      printf1( "ENOENT = %d\n", ENOENT );
      goto EXIT;
    }
  }
   
 EXIT:
  if( dataPtr->successCase ) {
    if (path)
      free( path );
  }
  printf5( "Returning from test\n" );
  return rc;
}
