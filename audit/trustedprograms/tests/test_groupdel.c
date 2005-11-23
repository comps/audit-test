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
 **  FILE       : test_groupdel.c
 **
 **  PURPOSE    : To test the groupdel trusted program.
 **
 **  DESCRIPTION: The test_groupdel() function builds into the
 **  laus_test framework to verify that the Linux Audit System
 **  accurately logs trusted program audit information for the
 **  "groupdel" program.
 **  
 **  This function performs a series of tests on the groupdel trusted
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

int test_groupdel(struct audit_data* dataPtr) {

  int rc = 0;
  int test = 1;
  char* command;
  char* group;
  int gid;
  FILE* fPtr;


  if (( rc = ShadowTestSetup(1) == -1 )) {
      goto EXIT;
  }

  // Test 1:
  // When a group is deleted.
  // The record is generated with the following commands:
  // groupdel
  // 
  // In addition to the standard audit information, the following string will be logged:
  // groupdel: group deleted - group=group, gid=gid, by=uid
  // 
  // group = the name of the group that was deleted
  // gid = the gid of the group that was deleted
  // by = the uid of the user executing the command
  // 
  /**
   * Test 1 written by Michael A. Halcrow <mike@halcrow.us>
   */
 //TEST_1:
  printf("  TEST %d\n", test++);

  // Setup
  dataPtr->euid = 0;
  createTempGroupName( &group, &gid );
  command = mysprintf( "/usr/sbin/groupadd -g %d %s", gid, group );
  if( ( rc = system( command ) ) == -1 ) {
    printf( "Error adding group [name %s; gid %d] prior to testing group deletion audit record\n",
	     group, gid );
    goto EXIT;
  }
  free( command );
  
  // Execution
  command = mysprintf( "/usr/sbin/groupdel %s", group );

  dataPtr->comm = mysprintf( "groupdel: op=deleting group acct=%s res=success", group );
  runTrustedProgramWithoutVerify( dataPtr, command );
  verifyTrustedProgram( dataPtr );
  
  // Cleanup
  free( dataPtr->comm );
  free( command );
  //
  // End Test 1

  free( group );

  rc = 0;

 EXIT:

  restoreFile("/etc/default/useradd");

  printf("Returning from test_groupdel()\n");
  return rc;
}


