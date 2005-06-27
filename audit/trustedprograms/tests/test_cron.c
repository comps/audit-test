
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
 **  FILE       : test_cron.c
 **
 **  PURPOSE    : To test the cron trusted program.
 **
 **  DESCRIPTION: The test_cron() function builds into the
 **  laus_test framework to verify that the Linux Audit System
 **  accurately logs trusted program audit information for the
 **  "cron" program.
 **  
 **  This function performs a series of tests on the cron trusted
 **  program.  Each subtest has its own setup, execution, and cleanup.
 **  The setup may consist of testing for the existence or non-existence
 **  of entities, the dynamic creation of entities, as well as the 
 **  definition of the expected text to be found in the audit.
 **  Each subtest is documented below to describe its own particulars.
 **
 **  HISTORY    :
 **    07/2003 Originated by Dustin Kirkland <k1rkland@us.ibm.com>
 **    07/2003 Adapted by Michael A. Halcrow <mike@halcrow.us>
 **
 **********************************************************************/

#include "includes.h"
#include "trustedprograms.h"
#include <time.h>

int test_cron(laus_data* dataPtr) {

  int rc = 0;
  int test = 1;
  char* command;
  char* crontab = "tabs/root"; // TODO: Verify
  char* user = "root";
  char* cmd = "/bin/true";
  int uid = 0;
  int gid = 0;
  //FILE* fPtr;


  // Test 1:
  // When cron is started.
  // The record is generated with the following commands:
  // cron
  // 
  // In addition to the standard audit information, the following string will be logged:
  // cron: started
  // 
  /**
   * Test 1 written by Michael A. Halcrow <mike@halcrow.us>
   */
 //TEST_1:
  printf5("  TEST %d\n", test++);

  // Setup
  dataPtr->msg_euid = 0;
  dataPtr->msg_egid = 0;
  // Start by killing the currently running cron job
  command = mysprintf("kill `cat /var/run/cron.pid`");
  if( ( rc = system( command ) ) == -1 ) {
    printf( "Error killing cron daemon\n" );
  }
  free(command);
  backupFile("/etc/crontab");
  backupFile("/etc/cron.d/sysstat");
  backupFile("/etc/cron.d/seccheck");
  RUNCOMMANDORDIE("rm -f /etc/crontab; touch /etc/crontab");
  RUNCOMMANDORDIE("rm -f /etc/cron.d/sysstat; touch /etc/cron.d/sysstat");
  RUNCOMMANDORDIE("rm -f /etc/cron.d/seccheck; touch /etc/cron.d/seccheck");
  //Fail silently, "crontab -r" may fail due to root not having a crontab
  RUNCOMMAND("crontab -l > crontab.bak; crontab -r");
  RUNCOMMANDORDIE("crontab crontab.bak");

  // Execution
  command = mysprintf( "/usr/sbin/cron" );
  runTrustedProgramWithoutVerify(dataPtr, command );
  dataPtr->msg_pid = getPid("cron");

  // Check associated audit records
  dataPtr->laus_var_data.textData.data = mysprintf( "cron: started" );
  verifyTrustedProgram( dataPtr );
  free( dataPtr->laus_var_data.textData.data );

  dataPtr->laus_var_data.textData.data = mysprintf( "cron: loading crontab - crontab=/etc/crontab, user=*system*" );
  verifyTrustedProgram( dataPtr );
  free( dataPtr->laus_var_data.textData.data );

  dataPtr->laus_var_data.textData.data = mysprintf( "cron: crontab loaded - crontab=/etc/crontab, user=*system*" );
  verifyTrustedProgram( dataPtr );
  free( dataPtr->laus_var_data.textData.data );

  dataPtr->laus_var_data.textData.data = mysprintf( "cron: loading crontab - crontab=tabs/root, user=root" );
  verifyTrustedProgram( dataPtr );
  free( dataPtr->laus_var_data.textData.data );

  dataPtr->laus_var_data.textData.data = mysprintf( "cron: crontab loaded - crontab=tabs/root, user=root" );
  verifyTrustedProgram( dataPtr );
  free( dataPtr->laus_var_data.textData.data );

  
  // Cleanup
  free( command );
  restoreFile("/etc/crontab");
  restoreFile("/etc/cron.d/sysstat");
  RUNCOMMANDORDIE("crontab crontab.bak");
  unlink("crontab.bak");
  // End Test 1

  // Test 2:
  // When a cron table is deleted.
  // The record is generated when the crontab command has deleted a cron table or system cron tables have been deleted.
  // 
  // In addition to the standard audit information, the following string will be logged:
  // cron: crontab deleted - crontab=crontab
  // 
  // crontab = the name of the cron table
  // 
  /**
   * Test 2 written by Michael A. Halcrow <mike@halcrow.us>
   */
 //TEST_2:
  printf5("  TEST %d\n", test++);
  // Setup
  dataPtr->msg_euid = 0;
  dataPtr->msg_egid = 0;
  system( "/usr/bin/crontab -l > crontab.bak" );
  system("echo \"* * * * * /bin/true\" > ct");
  system("/usr/bin/crontab ct");
  sleep(70);  // Must wait > 1 minute for cron daemon to pick up changes
  command = mysprintf( "/usr/bin/crontab -r" );
  dataPtr->laus_var_data.textData.data = mysprintf( "cron: crontab deleted - crontab=%s", crontab );

  // Execution
  runTrustedProgramWithoutVerify(dataPtr, command );
  sleep(70);  // Must wait > 1 minute for cron daemon to pick up changes
  dataPtr->end_r_time = time( NULL );
  dataPtr->msg_pid = getPid("cron");
  verifyTrustedProgram( dataPtr );
  
  // Cleanup
  free( command );
  free( dataPtr->laus_var_data.textData.data );
  system( "/usr/bin/crontab crontab.bak" );
  unlink("ct");
  unlink("crontab.bak");
  // End Test 2



  // Test 3:
  // When a cron table is modified.
  // The record is generated when the crontab command has modified a cron table or system cron tables have been modified.
  // 
  // In addition to the standard audit information, the following string will be logged:
  // cron: crontab modified - crontab=crontab
  // 
  // crontab = the name of the cron table
  // 
  /**
   * Test 3 written by Michael A. Halcrow <mike@halcrow.us>
   */
/*
 TEST_3:
  printf5("  TEST %d\n", test++);
  // Setup
  dataPtr->msg_euid = 0;
  dataPtr->msg_egid = 0;
  system( "/usr/bin/crontab -l > crontab.bak" );
  system("/usr/bin/crontab -l > crontab.new");
  system("echo \"* * * * * /bin/true\" >> crontab.new");
  system("/usr/bin/crontab -r");
  command = mysprintf( "/usr/bin/crontab crontab.new" );
  dataPtr->laus_var_data.textData.data = mysprintf( "cron: crontab modified -crontab=%s", crontab );

  // Execution
  sleep(70);
  runTrustedProgramWithoutVerify(dataPtr, command );
  sleep(70);  // Must wait > 1 minute for cron daemon to pick up changes
  dataPtr->end_r_time = time( NULL );
  dataPtr->msg_pid = getPid("cron");
  verifyTrustedProgram( dataPtr );

  // Cleanup
  free( command );
  free( dataPtr->laus_var_data.textData.data );
  system("/usr/bin/crontab -r");
  system( "/usr/bin/crontab crontab.bak" );
  unlink("crontab.new");
  unlink("crontab.bak");
  // End Test 3
*/

  // Test 4:
  // When a cron table is loading.
  // The record is generated whenever a cron table is being loaded.  This could be the result of cron starting or cron table being added or modified.
  // 
  // In addition to the standard audit information, the following string will be logged:
  // cron: loading crontab - crontab=crontab, user=name
  // 
  // crontab = the name of the cron table
  // user = the name of the cron table owner
  // 		("*system*" for system tables)
  // 
  /**
   * Test 4 written by Michael A. Halcrow <mike@halcrow.us>
   */
 //TEST_4:
  printf5("  TEST %d\n", test++);
 
  // Setup 
  command = "kill `cat /var/run/cron.pid`";
  if( ( rc = system( command ) ) == -1 ) {
    printf( "Error killing cron job\n" );
  }
  command = mysprintf( "/usr/sbin/cron" );
  dataPtr->laus_var_data.textData.data = mysprintf( "cron: loading crontab - crontab=%s, user=%s",
						    crontab, user );

  // Execution
  runTrustedProgramWithoutVerify(dataPtr, command );
  sleep(70);  // Must wait > 1 minute for cron daemon to pick up changes
  dataPtr->end_r_time = time( NULL );
  dataPtr->msg_pid = getPid("cron");
  verifyTrustedProgram( dataPtr );
  
  // Cleanup
  free( command );
  free( dataPtr->laus_var_data.textData.data );
  // End Test 4

  // Test 5:
  // When a cron table entry is loaded.
  // The record is generated whenever a cron table entry is loaded.  This could be the result of cron starting or cron table being added or modified.
  // 
  // In addition to the standard audit information, the following string will be logged:
  // cron: crontab entry loaded -uid=uid, gid=gid, cmd=string
  // 
  // uid = the uid to be associated with the command
  // gid = the gid to be associated with the command
  // cmd = the command that will be executed
  // 
  /**
   * Test 5 written by Michael A. Halcrow <mike@halcrow.us>
   */
 //TEST_5:
  printf5("  TEST %d\n", test++);
  // Setup
  dataPtr->msg_euid = 0;
  dataPtr->msg_egid = 0;
  system( "/usr/bin/crontab -l > crontab.bak" );
  system("echo \"* * * * * /bin/true\" > ct");

  command = mysprintf( "/usr/bin/crontab ct" );
  dataPtr->laus_var_data.textData.data = mysprintf( "cron: crontab entry loaded - uid=%d, gid=%d, cmd=%s",
						    uid, gid, cmd );
  // Execution
  runTrustedProgramWithoutVerify(dataPtr, command );
  sleep(70);  // Must wait > 1 minute for cron daemon to pick up changes
  dataPtr->end_r_time = time( NULL );
  dataPtr->msg_pid = getPid("cron");
  verifyTrustedProgram( dataPtr );
  
  // Cleanup
  free( command );
  free( dataPtr->laus_var_data.textData.data );
  system( "/usr/bin/crontab crontab.bak" );
  unlink("ct");
  unlink("crontab.bak");
  // End Test 5

  // Test 6:
  // When a cron table is loaded.
  // The record is generated whenever a cron table has been loaded.  This could be the result of cron starting or cron table being added or modified.
  // 
  // In addition to the standard audit information, the following string will be logged:
  // cron: crontab loaded - crontab=crontab, user=name,
  // 
  // crontab = the name of the cron table
  // user = the name of the cron table owner
  // 		("*system*" for system tables)
  // 
  /**
   * Test 6 written by Michael A. Halcrow <mike@halcrow.us>
   */
 //TEST_6:
  printf5("  TEST %d\n", test++);
  // Setup
  dataPtr->msg_euid = 0;
  dataPtr->msg_egid = 0;
  command = "/etc/init.d/crond stop || true";
  if( ( rc = system( command ) ) == -1 ) {
    printf( "Error killing cron job\n" );
  }
  command = mysprintf( "/usr/sbin/cron" );
  dataPtr->laus_var_data.textData.data = mysprintf( "cron: crontab loaded - crontab=%s, user=%s",
						    crontab, user );

  // Execution
  runTrustedProgramWithoutVerify(dataPtr, command );
  sleep(70);  // Must wait > 1 minute for cron daemon to pick up changes
  dataPtr->end_r_time = time( NULL );
  dataPtr->msg_pid = getPid("cron");
  verifyTrustedProgram( dataPtr );
  
  // Cleanup
  free( command );
  free( dataPtr->laus_var_data.textData.data );
  // End Test 6

  // Test 7:
  // When a cron job is executed.
  // The record is generated when a cron table job is executed.
  // 
  // In addition to the standard audit information, the following string will be logged:
  // cron: executing cron job - crontab=crontab, uid=uid, gid=gid, cmd=cmd
  // 
  // crontab = the name of the cron table
  // cmd = the command that will be executed
  // uid = the uid associated with the command
  // gid = the gid associated with the command
  // 
  /**
   * Test 7 written by Michael A. Halcrow <mike@halcrow.us>
   */
 //TEST_7:
  printf5("  TEST %d\n", test++);
  // Setup
  dataPtr->msg_euid = 0;
  dataPtr->msg_egid = 0;
  system( "/usr/bin/crontab -l > crontab.bak" );
  system("echo \"* * * * * /bin/true\" > ct");
  command = mysprintf( "/usr/bin/crontab ct" );
  dataPtr->laus_var_data.textData.data = mysprintf( "cron: executing cron job - crontab=%s, uid=%d, gid=%d, cmd=%s", crontab, dataPtr->msg_euid, dataPtr->msg_egid, cmd );

  // Execution
  runTrustedProgramWithoutVerify(dataPtr, command );
  sleep(70);  // Must wait > 1 minute for cron daemon to pick up changes
  dataPtr->end_r_time = time( NULL );
  dataPtr->msg_pid = NO_PID_CHECK; // pid reported by audit is pid of program executed
  verifyTrustedProgram( dataPtr );

  // Cleanup
  free( command );
  free( dataPtr->laus_var_data.textData.data );
  system( "/usr/bin/crontab crontab.bak" );
  unlink("ct");
  unlink("crontab.bak");
  // End Test 7

 EXIT_CLEANUP:  

 //EXIT:
  printf5("Returning from test_cron()\n");
  return rc;
}


