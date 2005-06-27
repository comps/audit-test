
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
 **  FILE       : test_crontab.c
 **
 **  PURPOSE    : To test the crontab trusted program.
 **
 **  DESCRIPTION: The test_crontab() function builds into the
 **  laus_test framework to verify that the Linux Audit System
 **  accurately logs trusted program audit information for the
 **  "crontab" program.
 **  
 **  This function performs a series of tests on the crontab trusted
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

#include "includes.h"
#include "trustedprograms.h"

int test_crontab(laus_data* dataPtr) {

  int rc = 0;
  int test = 1;
  char* command;

  dataPtr->msg_euid = 0;
  dataPtr->msg_egid = 0;

  // Test 1:
  // When a cron table is listed.
  // The record is generated with the following commands:
  // crontab -l
  // 
  // In addition to the standard audit information, the following string will be logged:
  // crontab: crontab listed - crontab=table, owner=uid, by=uid
  // 
  // crontab = the name of the cron table
  // owner = the uid of the cron table owner
  // by = the uid of the user executing the command
  // 
  /**
   * Test 1 written by Michael A. Halcrow <mike@halcrow.us>
   */
 //TEST_1:
  printf5("  TEST %d\n", test++);

  // Execute command
  command = mysprintf( "/usr/bin/crontab -l" );
  dataPtr->laus_var_data.textData.data = 
    mysprintf( "crontab: crontab listed - crontab=tabs/root, owner=0, by=0" );
  runTrustedProgramWithoutVerify( dataPtr, command );
  verifyTrustedProgram( dataPtr );

  //Cleanup
  free( command );
  free( dataPtr->laus_var_data.textData.data );

  // End Test 1


  // Test 2:
  // When a cron table is deleted.
  // The record is generated with the following commands:
  // crontab -r
  // 
  // In addition to the standard audit information, the following string will be logged:
  // crontab: crontab deleted - crontab=table, owner=uid, by=uid
  // 
  // crontab = the name of the cron table
  // owner = the uid of the cron table owner
  // by = the uid of the user executing the command
  // 
  /**
   * Test 2 written by Michael A. Halcrow <mike@halcrow.us>
   */
 //TEST_2:
  printf5("  TEST %d\n", test++);
  
  RUNCOMMANDORDIE( "/usr/bin/crontab -l > ct" );

  // Execute command
  command = mysprintf( "/usr/bin/crontab -r" );
  dataPtr->laus_var_data.textData.data = 
    mysprintf( "crontab: crontab deleted - crontab=tabs/root, owner=0, by=0" );
  runTrustedProgramWithoutVerify( dataPtr, command );
  verifyTrustedProgram( dataPtr );

  //Cleanup
  free( command );
  free( dataPtr->laus_var_data.textData.data );

  RUNCOMMANDORDIE( "/usr/bin/crontab ct" );

  // End Test 2

  // Test 3:
  // When a cron table is added/replaced.
  // The record is generated with the following commands:
  // crontab
  // crontab -e
  // 
  // In addition to the standard audit information, the following string will be logged:
  // crontab: crontab replaced - crontab=table, owner=uid, by=uid
  // 
  // crontab = the name of the cron table
  // owner = the uid of the cron table owner
  // by = the uid of the user executing the command
  // 
  /**
   * Test 3 written by Michael A. Halcrow <mike@halcrow.us>
   */
 //TEST_3:
  printf5("  TEST %d\n", test++);
  
  RUNCOMMANDORDIE( "/usr/bin/crontab -l > ct" );

  // Execute command
  command = mysprintf( "/usr/bin/crontab ct" );
  dataPtr->laus_var_data.textData.data = 
    mysprintf( "crontab: crontab replaced - crontab=tabs/root, owner=0, by=0" );
  runTrustedProgramWithoutVerify( dataPtr, command );
  verifyTrustedProgram( dataPtr );

  //Cleanup
  free( command );
  free( dataPtr->laus_var_data.textData.data );

  // End Test 3

  // Test 4:
  // When executing the command without the necessary permission.
  // The permission is determined through the use of the allow and deny files and their contents.  Refer to the man page for the crontab command for further information.  The record is generated with the following commands:
  // crontab
  // 
  // In addition to the standard audit information, the following string will be logged:
  // crontab: permission denied - owner=uid, by=uid
  // 
  // owner = the uid of the cron table owner
  // by = the uid of the user executing the command
  // 
  /**
   * Test 4 written by Michael A. Halcrow <mike@halcrow.us>
   */
 //TEST_4:
  printf5("  TEST %d\n", test++);

  backupFile( "/var/spool/cron/allow" );
  backupFile( "/var/spool/cron/deny" );

  if( chdir( cwd ) == -1 ) {
    printf1( "Error changing to working directory [%s]: errno = [%i]\n", cwd, errno );
    goto EXIT_CLEANUP;
  }

  RUNCOMMANDORDIE( "rm -f /var/spool/cron/allow" );
  RUNCOMMANDORDIE( "echo \"root\" > /var/spool/cron/deny" );

  // Execute command
  command = mysprintf( "/usr/bin/crontab -l" );
  dataPtr->laus_var_data.textData.data = 
    mysprintf( "crontab: permission denied - owner=0, by=0" );
  runTrustedProgramWithoutVerify( dataPtr, command );
  verifyTrustedProgram( dataPtr );

  //Cleanup
  free( command );
  free( dataPtr->laus_var_data.textData.data );

  restoreFile( "/var/spool/cron/allow" );
  restoreFile( "/var/spool/cron/deny" );

  // End Test 4

 EXIT_CLEANUP:  

 //EXIT:
  printf5("Returning from test_crontab()\n");
  return rc;
}


