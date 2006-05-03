/**********************************************************************
 **   Copyright (C) International Business Machines  Corp., 2003
 **   Copyright (C) Hewlett-Packard Development Company, L.P., 2005
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
 **    12/2005 Adapted by Matt Anderson <mra@hp.com>
 **
 **********************************************************************/

#include "includes.h"
#include "trustedprograms.h"

int test_groupmod(struct audit_data* dataPtr) {

  int rc = 0;
  int test = 1;
  char* command;
  char* Group;
  int gid;
  int new_gid;

/*
  if (( rc = ShadowTestSetup(1) == -1 )) {
      goto EXIT;
  }
*/

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
  dataPtr->type = AUDIT_MSG_USER;
  dataPtr->comm = mysprintf( "groupmod: op=modifing group acct=%s res=success", Group );
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

 EXIT:

  restoreFile("/etc/default/useradd");

  printf("Returning from test_groupmod()\n");
  return !!fail_testcases;
}


