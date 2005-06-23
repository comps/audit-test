
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
 **  FILE       : test_at.c
 **
 **  PURPOSE    : To test the at trusted program.
 **
 **  DESCRIPTION: The test_at() function builds into the
 **  laus_test framework to verify that the Linux Audit System
 **  accurately logs trusted program audit information for the
 **  "at" program.
 **  
 **  This function performs a series of tests on the at trusted
 **  program.  Each subtest has its own setup, execution, and cleanup.
 **  The setup may consist of testing for the existence or non-existence
 **  of entities, the dynamic creation of entities, as well as the 
 **  definition of the expected text to be found in the audit.
 **  Each subtest is documented below to describe its own particulars.
 **
 **  HISTORY    :
 **    07/2003 Originated by Dustin Kirkland <k1rkland@us.ibm.com>
 **
 **********************************************************************/

#include "trustedprograms.h"
#include <time.h>
#include <pwd.h>
#include <grp.h>

int test_at(laus_data* dataPtr) {

  int rc = 0;
  int test = 1;
  char* command;
  time_t cur_time;
  char timestr[64];
  int job;
  //int fd;
  FILE* fPtr;
  int uid, gid;
  int pid = 0;
  int trustedGid;

  struct passwd* passwd_data = NULL;
  struct group* group_data = NULL;

  uid = dataPtr->msg_euid;
  gid = dataPtr->msg_egid;

  passwd_data = getpwuid(uid);

  if (( pid = getPid( "atd" ) < 0 )) {
      if (( rc = system("/usr/sbin/atd") == -1 )) {
	  printf1("Unable to start atd cannot execute at tests\n");
	  goto EXIT;
      }
  }

  if ((group_data = getgrnam("trusted")) == NULL) {
    printf1("ERROR: Unable to get trusted group info.\n");
    goto EXIT;
  }
  trustedGid = group_data->gr_gid;

  // Test 1:
  // When a job is created.
  // The record is generated with the following commands:
  // at
  // 
  // batch
  // 
  // In addition to the standard audit information, the following string will be logged:
  // at: job created - queue=queue, job=job#, time=jobtime, by=uid
  // 
  // queue = the job queue
  // job = the job number
  // time = the time that the job is scheduled for
  // by = the uid of the user executing the command
  // 
 //TEST_1:
  printf5("  TEST %d\n", test++);

  // Setup
  dataPtr->msg_euid = 0;
  dataPtr->msg_egid = 0;

  // get job number
  fPtr = fopen("/var/spool/atjobs/.SEQ", "r");
  fscanf( fPtr, "%x", &job );
  fclose( fPtr );
  job++;

  // get current time string
  if (( rc = ((int)time(&cur_time)) == -1 )) {
      printf1("Cannot get time (errno=%d), cannot create at command\n", errno);
      goto EXIT;
  }
  strftime(timestr, sizeof(timestr), "%Y-%m-%d %H:%M", localtime(&cur_time));

  dataPtr->laus_var_data.textData.data = mysprintf("at: job created - queue=a, job=%d, time=%s, by=%d", job, timestr, dataPtr->msg_euid);
  command = mysprintf("/usr/bin/at -f /bin/true now");

  // Execution
  runTrustedProgramWithoutVerify(dataPtr, command );
  verifyTrustedProgram( dataPtr );

  // Cleanup
  free( dataPtr->laus_var_data.textData.data );
  free(command);

  // End Test 1


  // Test 2:
  // When executing the command without the necessary permission.
  // The permission is determined through the use of the at.allow and at.deny files and their contents.  Refer to the man page for the at command for further information.  The record is generated with the following commands:
  // at
  // atq
  // atrm
  // batch
  // 
  // In addition to the standard audit information, the following string will be logged:
  // at: permission denied - by=uid
  // 
  // by = the uid of the user executing the command
  // 
 //TEST_2:
  printf5("  TEST %d\n", test++);

  // Setup
  backupFile("/etc/at.deny");
  fPtr = fopen("/etc/at.deny", "a");
  fprintf(fPtr, "%s", passwd_data->pw_name);
  fclose(fPtr);

  // Execution
  dataPtr->laus_var_data.textData.data = mysprintf("at: permission denied - by=%d", uid);
  command = mysprintf("/usr/bin/at -f /bin/true now", passwd_data->pw_name);

  dataPtr->msg_euid = uid;   // effective-uid is the test user
  dataPtr->msg_egid = trustedGid;   // effective-uid is the test user
  dataPtr->msg_ruid = uid;   // real-uid is the test user
  dataPtr->msg_rgid = trustedGid;   // real-gid is the test user
//  dataPtr->msg_fsgid = gid;  // fs-gid is the test user

  runTrustedProgramWithoutVerify(dataPtr, command );

  dataPtr->msg_euid = 0;   // at is a set-uid binary
  dataPtr->msg_suid = 0;
  dataPtr->msg_fsuid = 0;
//  dataPtr->msg_sgid = gid; // sgid because of forked process
  verifyTrustedProgram( dataPtr );

  // Cleanup
  restoreFile("/etc/at.deny");
  free( dataPtr->laus_var_data.textData.data );
  free(command);

  // End Test 2

  

 EXIT:
  system("killall atd");

  printf5("Returning from test_at()\n");
  return rc;
}


