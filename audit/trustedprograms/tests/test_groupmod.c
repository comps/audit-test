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
 **  FILE       : test_groupmod.c
 **
 **  PURPOSE    : To test the groupmod trusted program.
 **
 **  DESCRIPTION: The test_groupmod() function builds into the
 **  laus_test framework to verify that the Linux Audit System
 **  accurately logs trusted program audit information for the
 **  "groupmod" program.
 **  
 **  This function performs a series of tests on the groupmod trusted
 **  program.  Each subtest has its own setup, execution, and cleanup.
 **  The setup may consist of testing for the existence or non-existence
 **  of entities, the dynamic creation of entities, as well as the 
 **  definition of the expected text to be found in the audit.
 **  Each subtest is documented below to describe its own particulars.
 **
 **  HISTORY    :
 **    07/2003 Originated by Dustin Kirkland <k1rkland@us.ibm.com>
 **    07/2003 Furthered by Michael A. Halcrow <mike@halcrow.us>
 **
 **********************************************************************/

#include "includes.h"
#include "trustedprograms.h"

int test_groupmod(struct audit_data* dataPtr) {

  int rc = 0;
  int test = 1;
  char* command;
  //char* oldGroup;
  //char* newGroup; 
  char* Group;
  char* new_password;
  char* user;
  char* home;
  int uid;
  int gid;
  int new_gid;
  //int tmp;
  FILE* fPtr;

  if (( rc = ShadowTestSetup(1) == -1 )) {
      goto EXIT;
  }

  // Test 1:
  // When a group's gid is changed 
  // The record is generated with the following commands:
  // groupmod -g gid
  // 
  // In addition to the standard audit information, the following string will be logged:
  // groupmod: group gid changed - group=group, gid=gid, oldgid=gid, by=uid
  // 
  /**
   * Test 1 written by Jerone Young <jyoung5@us.ibm.com> 
   */
 //TEST_1:
  printf("  TEST %d\n", test++);

  // Setup
  dataPtr->euid = 0;
  createTempGroupName( &Group, &gid );
  new_gid=897; //Some random integer value number

  command = mysprintf( "/usr/sbin/groupadd -g %d %s", gid, Group );
  if( ( rc = system( command ) ) == -1 ) {
    printf( "Error adding group [name %s; gid %d] prior to testing group deletion audit record\n",
	     Group, gid );
    goto EXIT;
  }
  free( command );


  // Execution
  command = mysprintf( "/usr/sbin/groupmod -g %d %s", new_gid, Group );
  dataPtr->comm = mysprintf( "groupmod: group gid changed - group=%s, gid=%d, oldgid=%d, by=%d",
	        Group, new_gid, gid, dataPtr->euid );
  runTrustedProgramWithoutVerify( dataPtr, command );
  verifyTrustedProgram( dataPtr );

  // Cleanup
  free( dataPtr->comm );
  free( command );
  command = mysprintf( "/usr/sbin/groupdel %s", Group );
  if( ( rc = system( command ) ) == -1 ) {
    printf( "Error during cleanup of groupadd test 1: system() returned -1\n" );
    goto EXIT;
  }
  free( command );
  // End Test 1

  // Test 2:
  // When a groups's password is changed
  // The record is generated with the following command:
  // groupmod -p password
  // 
  // In addition to the standard audit information, the following string will be logged:
  // groupmod: group password changed - group=group, gid=gid, by=uid
  /**
   *     Test 2 written by Jerone Young <jyoung5@us.ibm.com>
   */
   //TEST_2:
   printf(" TEST %d\n", test++);
   
   // Setup
   dataPtr->euid = 0;
   createTempGroupName( &Group, &gid );  
   new_password = "7ijlxihgxUs"; //some random password 

  command = mysprintf( "/usr/sbin/groupadd -g %d %s", gid, Group );
  if( ( rc = system( command ) ) == -1 ) {
    printf( "Error adding group [name %s; gid %d] prior to testing group deletion audit record\n",
             Group, gid );
    goto EXIT;
  }
  free( command );

  // Execution
  command = mysprintf( "/usr/sbin/groupmod -p %s %s", new_password, Group );
  dataPtr->comm = mysprintf( "groupmod: group password changed - group=%s, gid=%d, by=%d",
                Group, gid, dataPtr->euid );
  runTrustedProgramWithoutVerify( dataPtr, command );
  verifyTrustedProgram( dataPtr );
 
  // Cleanup
  free( dataPtr->comm );
  free( command );
  command = mysprintf( "/usr/sbin/groupdel %s", Group );
  if( ( rc = system( command ) ) == -1 ) {
    printf( "Error during cleanup of groupadd test 1: system() returned -1\n" );
    goto EXIT;
  }
  free( command );
  //End Test 2 


  // Test 4:
  // When a user is added to a group and when user is removed from a group
  // groupmod -A username
  // groupmod -R username
  //
  // In addition to the statndard audit information, the following string will be logged:
  // groupmod: user added to group - user=user, group=group, gid=gid, by=uid
  // groupmod: user removed from group - user=user, group=group, gid=gid, by=uid
  //TEST_4:
  printf(" TEST %d\n", test++);
  
  // Setup
  dataPtr->euid = 0;
  createTempGroupName( &Group, &gid );
  createTempUserName( &user, &uid, &home );

  // Create user
  command = mysprintf( "/usr/sbin/useradd -u %d %s", uid, user );
  if( ( rc = system( command ) ) == -1 ) {
    printf( "Error creating user [%s]\n", user );
    goto EXIT;
  }
  free( command );
   
  // Create group
  command = mysprintf( "/usr/sbin/groupadd -g %d %s", gid, Group );
  if( ( rc = system( command ) ) == -1 ) {
    printf( "Error adding group [name %s; gid %d] prior to testing group deletion audit record\n",
             Group, gid );
    goto EXIT;
  }
  free( command );  

  // Execution
  command = mysprintf( "/usr/sbin/groupmod -A %s %s", user, Group);

  //check audit record
  dataPtr->comm = mysprintf( "groupmod: user added to group - user=%s, group=%s, gid=%d, by=%d",
                user, Group, gid, dataPtr->euid );
  runTrustedProgramWithoutVerify( dataPtr, command );
  verifyTrustedProgram( dataPtr );
  free( dataPtr->comm );
  free ( command );

  //execution
  command = mysprintf( "/usr/sbin/groupmod -R %s %s", user, Group);
  
  //check audit record
  dataPtr->comm = mysprintf( "groupmod: user removed from group - user=%s, group=%s, gid=%d, by=%d",
                user, Group, gid, dataPtr->euid );
  runTrustedProgramWithoutVerify( dataPtr, command );
  verifyTrustedProgram( dataPtr );

  // Cleanup
  free( dataPtr->comm );
  free( command );
  command = mysprintf( "/usr/sbin/groupdel %s", Group );
  if( ( rc = system( command ) ) == -1 ) {
    printf( "Error during cleanup of groupadd test 1: system() returned -1\n" );
    goto EXIT;
  }
  free( command );
  
  command = mysprintf( "/usr/sbin/userdel %s", user );
  if( ( rc = system( command ) ) == -1 ) {
    printf( "Error deleting user [%s]\n", user );
    goto EXIT;
  }
  free( command );
 

  // Test 5:
  // When PAM authentication fails for the user.
  // The program must be built with PAM authentication enabled.  The record is generated with the following commands:
  // groupmod
  // 
  // In addition to the standard audit information, the following string will be logged:
  // groupmod: PAM authentication failed - by=uid
  // 
  // by = the uid of the user executing the command
  /**
   * Test 5  written by Michael A. Halcrow <mike@halcrow.us>
            modified by Jerone Young <jeroney@us.ibm.com>
   */
 //TEST_5:
  printf("  TEST %d\n", test++);

  // Setup
  dataPtr->euid = 0;

  command = mysprintf( "/usr/sbin/groupadd -g %d %s", gid, Group );
  if( ( rc = system( command ) ) == -1 ) {
    printf( "Error adding group [name %s; gid %d] prior to testing group deletion audit record\n",
	     Group, gid );
    goto EXIT;
  }
  free( command );

  backupFile( "/etc/pam.d/shadow" );
  if( ( fPtr = fopen( "/etc/pam.d/shadow", "w" ) ) == NULL ) {
    printf( "Error opening /etc/pam.d/shadow for write w/ truncate access\n" );
    rc = -1;
    goto EXIT;
  }
  if( ( rc = fputs( "auth required pam_deny.so", fPtr ) ) == EOF ) {
    printf( "Error writing to /etc/pam.d/shadow\n" );
    goto EXIT;
  }
  if( ( rc = fclose( fPtr ) ) != 0 ) {
    printf( "Error closing file /etc/pam.d/shadow\n" );
    goto EXIT;
  }

  // Execution
  command = mysprintf( "/usr/sbin/groupmod -g %d %s", gid, Group );
  dataPtr->comm = mysprintf("groupmod: PAM authentication failed - by=%d",
	      dataPtr->euid );
  runTrustedProgramWithoutVerify( dataPtr, command );
  verifyTrustedProgram( dataPtr );

  // Cleanup
  free( dataPtr->comm );
  free( command );
  restoreFile( "/etc/pam.d/shadow" );
  command = mysprintf( "/usr/sbin/groupdel %s", Group );
  if( ( rc = system( command ) ) == -1 ) {
    printf( "Error during cleanup of groupadd test 1: system() returned -1\n" );
    goto EXIT;
  }
  free( command );
  
  // End Test 5 

 EXIT:

  restoreFile("/etc/default/useradd");

  printf("Returning from test_groupmod()\n");
  return rc;
}


