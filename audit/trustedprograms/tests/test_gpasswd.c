
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
 **  FILE       : test_gpasswd.c
 **
 **  PURPOSE    : To test the gpasswd trusted program.
 **
 **  DESCRIPTION: The test_gpasswd() function builds into the
 **  laus_test framework to verify that the Linux Audit System
 **  accurately logs trusted program audit information for the
 **  "gpasswd" program.
 **  
 **  This function performs a series of tests on the gpasswd trusted
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
#include "context.h"
#include <sys/ipc.h>
#include <sys/shm.h>
#include <asm/page.h>
#include <grp.h>

int test_gpasswd(audit_data* dataPtr) {

  int rc = 0;
  int test = 1;
  char* command;
  char* user;
  char* home;
  char* group;
  char* supGroup;
  char* secondSupGroup;
  int uid;
  int gid;
  int supGid;
  int secondSupGid;
  int trustedGid;
  int uidSave = dataPtr->euid;
  //int gidSave = dataPtr->egid;

  int shmid = -1;
  int* pidShared = 0;

  struct group* group_data = NULL;

  dataPtr->euid = 0;
  dataPtr->egid = 0;

  
  if (( rc = ShadowTestSetup(1) == -1 )) {
      goto EXIT;
  }

  if ((group_data = getgrnam("trusted")) == NULL) {
    printf("ERROR: Unable to get trusted group info.\n");
    goto EXIT;
  }

  trustedGid = group_data->gr_gid;
  
  createTempUserName( &user, &uid, &home );

  createTempGroupName( &group, &gid );

  // Create group
  command = mysprintf( "/usr/sbin/groupadd -g %d %s", gid, group );
  if( ( rc = system( command ) ) == -1 ) {
    printf( "Error creating group [%s]\n", group );
    goto EXIT;
  }
  free( command );    

  createTempGroupName( &supGroup, &supGid );

  // Create supplementary group
  command = mysprintf( "/usr/sbin/groupadd -g %d %s", supGid, supGroup );
  if( ( rc = system( command ) ) == -1 ) {
    printf( "Error creating group [%s]\n", supGroup );
    goto EXIT;
  }
  free( command );    

  createTempGroupName( &secondSupGroup, &secondSupGid );

  // Create second supplementary group
  command = mysprintf( "/usr/sbin/groupadd -g %d %s", secondSupGid, secondSupGroup );
  if( ( rc = system( command ) ) == -1 ) {
    printf( "Error creating group [%s]\n", secondSupGroup );
    goto EXIT;
  }
  free( command );    

  // Remove groups

  // Remove group
  command = mysprintf( "/usr/sbin/groupdel %s", group );
  if( ( rc = system( command ) ) == -1 ) {
    printf( "Error deleting group [%s]\n", group );
    goto EXIT;
  }
  free( command );    

  // Remove supplemantary group
  command = mysprintf( "/usr/sbin/groupdel %s", supGroup );
  if( ( rc = system( command ) ) == -1 ) {
    printf( "Error deleting group [%s]\n", supGroup );
    goto EXIT;
  }
  free( command );    

  // Remove second supplementary group
  command = mysprintf( "/usr/sbin/groupdel %s", secondSupGroup );
  if( ( rc = system( command ) ) == -1 ) {
    printf( "Error deleting group [%s]\n", secondSupGroup );
    goto EXIT;
  }
  free( command );

  // Get some shared memory
  if( ( shmid = shmget( IPC_PRIVATE, PAGE_SIZE, 0 ) ) == -1 ) {
    printf( "Error getting shared memory\n" );
    goto EXIT_CLEANUP;
  }
  if( (long)( pidShared = (int*)shmat( shmid, NULL, 0 ) ) == -1 ) {
    printf( "Error attaching to shared memory segment with shmid = [%d]: errno = [%i]\n", 
	     shmid, errno );
    goto EXIT_CLEANUP;
  }

  // Test 1:
  // When attempting to modify a group when the user is not "root" or an administrator.  There are multiple ways to generate this entry depending on how the command is built.  The defines SHADOWGRP and FIRST_MEMBER_IS_ADMIN define the code used to generate this entry.
  // The record is generated with the following commands:
  // gpasswd
  // 
  // In addition to the standard audit information, the following string will be logged:
  // gpasswd: group modification denied - group=group, by=uid
  // 
  // group = the name of the group being modified
  // by = the uid of the user executing the command
  // 
  /**
   * Test 1 written by Michael A. Halcrow <mike@halcrow.us>
   */
 //TEST_1:
  printf("  TEST %d\n", test++);
  // Create a group
  RUNCOMMANDORDIE( "/usr/sbin/groupadd -g %d %s", gid, group );

  // Execute: remove the password from the group
  dataPtr->uid = dataPtr->euid = uidSave;
  dataPtr->gid = dataPtr->egid = trustedGid;
  command = mysprintf( "/usr/bin/gpasswd %s", group );
  dataPtr->comm = mysprintf( "gpasswd: group modification denied - group=%s, by=%d", 
	       group, dataPtr->euid );
  runTrustedProgramWithoutVerify(dataPtr, command);

  //setuid program funkiness
  dataPtr->suid = dataPtr->fsuid = dataPtr->euid = 0;

  verifyTrustedProgram( dataPtr );

  dataPtr->uid = dataPtr->euid = 0;
  dataPtr->gid = dataPtr->egid = 0;

  // Cleanup
  free( dataPtr->comm );
  free( command );
  RUNCOMMANDORDIE( "/usr/sbin/groupdel %s", group );  
  // End Test 1

  // Test 2:
  // When removing the password associated with a group.
  // The record is generated with the following commands:
  // gpasswd -r
  // 
  // In addition to the standard audit information, the following string will be logged:
  // gpasswd: group password removed - group=group, by=uid
  // 
  // group = the name of the group being modified
  // by = the uid of the user executing the command
  // 
  /**
   * Test 2 written by Michael A. Halcrow <mike@halcrow.us>
   */
 //TEST_2:
  printf("  TEST %d\n", test++);
  // Create a group
  RUNCOMMANDORDIE( "/usr/sbin/groupadd -g %d %s", gid, group );

  // Execute: remove the password from the group
  command = mysprintf( "/usr/bin/gpasswd -r %s", group );
  dataPtr->comm = mysprintf( "gpasswd: group password removed - group=%s, by=%d", 
	       group, dataPtr->euid );
  runTrustedProgramWithoutVerify( dataPtr, command );
  verifyTrustedProgram( dataPtr );

  // Cleanup
  free( dataPtr->comm );
  free( command );
  RUNCOMMANDORDIE( "/usr/sbin/groupdel %s", group );  
  // End Test 2

  // Test 3:
  // When the group password is changed.
  // The record is generated with the following commands:
  // gpasswd
  // 
  // In addition to the standard audit information, the following string will be logged:
  // gpasswd: group password changed - group=group, by=uid
  // 
  // group = the name of the group being modified
  // by = the uid of the user executing the command
  // 
  /**
   * Test 3 written by Michael A. Halcrow <mike@halcrow.us>
   */
 //TEST_3:
  printf("  TEST %d\n", test++);
  // Setup
  RUNCOMMANDORDIE( "/usr/sbin/groupadd -g %d %s", gid, group );

  // Execution
  command = mysprintf( "../utils/gpasswd_test_10.sh %s", 
		       group );
  
  //gpasswd uses another process to do this task
  dataPtr->pid = NO_PID_CHECK;

  dataPtr->comm = 
    mysprintf( "gpasswd: group password changed - group=%s, by=%d",
	       group, dataPtr->euid );
  runTrustedProgramWithoutVerify( dataPtr, command );
  verifyTrustedProgram( dataPtr );

  // Cleanup
  free( dataPtr->comm );
  free( command );
  RUNCOMMANDORDIE( "/usr/sbin/groupdel %s", group );  
  // End Test 3 

 EXIT_CLEANUP:

  free( user );
  free( home );
  free( group );
  free( supGroup );
  free( secondSupGroup );

  if( shmdt( pidShared ) == -1 ) {
    printf( "Error detaching from shared memory segment: errno = [%i]\n", errno );
    goto EXIT_CLEANUP;
  }
  if( shmctl( shmid, IPC_RMID, NULL ) == -1 ) {
    printf( "Error removing shared memory with shmid = [%d]: errno = [%i]\n",
	     shmid, errno );
    goto EXIT_CLEANUP;
  }

 EXIT:

  restoreFile("/etc/default/useradd");

  printf("Returning from test_gpasswd()\n");
  return rc;
}


