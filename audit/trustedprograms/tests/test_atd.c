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
 **  FILE       : test_atd.c
 **
 **  PURPOSE    : To test the atd trusted program.
 **
 **  DESCRIPTION: The test_atd() function builds into the
 **  laus_test framework to verify that the Linux Audit System
 **  accurately logs trusted program audit information for the
 **  "atd" program.
 **  
 **  This function performs a series of tests on the atd trusted
 **  program.  Each subtest has its own setup, execution, and cleanup.
 **  The setup may consist of testing for the existence or non-existence
 **  of entities, the dynamic creation of entities, as well as the 
 **  definition of the expected text to be found in the audit.
 **  Each subtest is documented below to describe its own particulars.
 **
 **  HISTORY    :
 **    07/2003 Originated by Dustin Kirkland <k1rkland@us.ibm.com>
 **    08/2003 Adapted by Michael A. Halcrow <mike@halcrow.us>
 **
 **********************************************************************/

#include "trustedprograms.h"

int test_atd(laus_data* dataPtr) {

  int rc = 0;
  int test = 1;
  char* command;
  //int fd;
  FILE* fPtr;
  int job;
  int pid;

  // Test 1:
  // When atd is started.
  // The record is generated with the following commands:
  // atd
  // 
  // In addition to the standard audit information, the following string will be logged:
  // atd: started
  // 
  /**
   * Test 1 written by Michael A. Halcrow <mike@halcrow.us>
   */
 //TEST_1:
  printf5("  TEST %d\n", test++);

  // Setup
  dataPtr->msg_euid = dataPtr->msg_ruid = 0;
  dataPtr->msg_egid = dataPtr->msg_rgid = 0;
  dataPtr->laus_var_data.textData.data = mysprintf( "atd: started" );
  command = mysprintf( "/usr/sbin/atd" );

  // Start by killing the currently running atd
  RUNCOMMANDORDIE("/etc/init.d/atd stop || true");

  // Execution
  runTrustedProgramWithoutVerify(dataPtr, command );
  // After starting up atd, get pid for audit record verification
  dataPtr->msg_pid = getPid( "atd" );
  // atd is a daemon, and runs as daemon user (2)
  dataPtr->msg_ruid = 25;
  dataPtr->msg_rgid = 25;
  verifyTrustedProgram( dataPtr );
  
  // Cleanup
  free( command );
  free( dataPtr->laus_var_data.textData.data );
  // End Test 1

  // Test 2:
  // When atd terminates.
  // In addition to the standard audit information, the following string will be logged:
  // atd: terminated
  // 
  /**
   * Test 2 written by Michael A. Halcrow <mike@halcrow.us>
   */
 //TEST_2:
  printf5("  TEST %d\n", test++);
  // Setup
  dataPtr->msg_euid = dataPtr->msg_ruid = 0;
  dataPtr->msg_egid = dataPtr->msg_rgid = 0;
  dataPtr->laus_var_data.textData.data = mysprintf( "atd: terminated" );

  // Start by ensuring that atd is running
  RUNCOMMANDORDIE( "/usr/sbin/atd" );

  // Execution
  // After starting up atd, get pid for audit record verification
  pid = getPid( "atd" );
  command = mysprintf( "/bin/kill %d", pid );
  runTrustedProgramWithoutVerify( dataPtr, command );
  // atd is a daemon, and runs as daemon user (2)
  dataPtr->msg_pid = pid;
  dataPtr->msg_ruid = 25;
  dataPtr->msg_rgid = 25;
  verifyTrustedProgram( dataPtr );
  
  // Cleanup
  free( command );
  free( dataPtr->laus_var_data.textData.data );
  // End Test 2

  // Test 3:
  // When atd is started for batch queue processing.
  // The record is generated with the following commands:
  // atd -s
  // 
  // In addition to the standard audit information, the following string will be logged:
  // atd: batch queue processing started
  // 
  /**
   * Test 3 written by Michael A. Halcrow <mike@halcrow.us>
   */
 //TEST_3:
  printf5("  TEST %d\n", test++);
  // Setup
  dataPtr->msg_euid = dataPtr->msg_ruid = 0;
  dataPtr->msg_egid = dataPtr->msg_rgid = 0;
  command = mysprintf( "/usr/sbin/atd -s" );
  dataPtr->laus_var_data.textData.data = mysprintf( "atd: batch queue processing started" );

  // Start by killing the currently running atd
  RUNCOMMANDORDIE("/etc/init.d/atd stop || true");

  // Execution
  runTrustedProgramWithoutVerify(dataPtr, command );
  // If there is nothing in the queue for batch processing, atd starts up and terminates
  //   immeadiately.  It is very difficult to obtain the PID of the atd process in this case.
  dataPtr->msg_pid = NO_PID_CHECK;
  // atd is a daemon, and runs as daemon user (2)
  dataPtr->msg_ruid = 25;
  dataPtr->msg_rgid = 25;
  verifyTrustedProgram( dataPtr );

  // Check for associated audit messages
  free( dataPtr->laus_var_data.textData.data );
  dataPtr->laus_var_data.textData.data = mysprintf( "atd: batch queue processing terminated" );
  verifyTrustedProgram( dataPtr );
  
  // Cleanup
  free( command );
  free( dataPtr->laus_var_data.textData.data );
  // End Test 3

  // Test 4:
  // When atd batch queue processing terminates.
  // In addition to the standard audit information, the following string will be logged:
  // atd: batch queue processing terminated
  // 
  /**
   * Test 4 written by Michael A. Halcrow <mike@halcrow.us>
   */
 //TEST_4:
  printf5("  TEST %d\n", test++);
  // Setup
  dataPtr->msg_euid = dataPtr->msg_ruid = 0;
  dataPtr->msg_egid = dataPtr->msg_rgid = 0;
  // If there are no queued batch jobs, atd should start up and terminate immediately
  command = mysprintf( "/usr/sbin/atd -s");
  dataPtr->laus_var_data.textData.data = mysprintf( "atd: batch queue processing started" );

  // Execution
  runTrustedProgramWithoutVerify( dataPtr, command );
  // If there is nothing in the queue for batch processing, atd starts up and terminates
  //   immeadiately.  It is very difficult to obtain the PID of the atd process in this case.
  dataPtr->msg_pid = NO_PID_CHECK;
  // atd is a daemon, and runs as daemon user (2)
  dataPtr->msg_ruid = 25;
  dataPtr->msg_rgid = 25;
  verifyTrustedProgram( dataPtr );

  // Check for associated audit messages
  free( dataPtr->laus_var_data.textData.data );
  dataPtr->laus_var_data.textData.data = mysprintf( "atd: batch queue processing terminated" );
  verifyTrustedProgram( dataPtr );

  
  // Cleanup
  free( command );
  free( dataPtr->laus_var_data.textData.data );
  RUNCOMMANDORDIE( "kill `pidof atd`; /usr/sbin/atd");
  // End Test 4

  // Test 5:
  // When a scheduled job is executed.
  // In addition to the standard audit information, the following string will be logged:
  // atd: executing job - queue=queue, job=job#, uid=uid, gid=gid
  // 
  // queue = the job queue
  // job = the job number
  // uid = the uid associated with the job
  // gid = the gid associated with the job
  // 
  // 
 //TEST_5:
  printf5("  TEST %d\n", test++);

  // Setup
  dataPtr->msg_euid = dataPtr->msg_ruid = 0;
  dataPtr->msg_egid = dataPtr->msg_rgid = 0;

  // Start by ensuring that atd is running
  RUNCOMMANDORDIE( "/usr/sbin/atd" );

  // get job number
  fPtr = fopen("/var/spool/atjobs/.SEQ", "r");
  fscanf(fPtr, "%x", &job);
  fclose(fPtr);
  job++;

  dataPtr->laus_var_data.textData.data = mysprintf("atd: executing job - queue=a, job=%d, uid=%d, gid=%d", job, dataPtr->msg_euid, dataPtr->msg_egid );
  command = mysprintf("/usr/bin/at -f /bin/true now");

  // Execution
  runTrustedProgramWithoutVerify(dataPtr, command );
  dataPtr->msg_pid = NO_PID_CHECK;
  dataPtr->msg_ruid = 25;
  dataPtr->msg_rgid = 25;
  verifyTrustedProgram( dataPtr );

  // Cleanup
  free( dataPtr->laus_var_data.textData.data );
  free(command);
  //
  // End Test 5

 EXIT_CLEANUP:  

 //EXIT:
  printf5("Returning from test_atd()\n");
  return rc;
}


