
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
 **  FILE       : test_sshd.c
 **
 **  PURPOSE    : To test the sshd PAM program.
 **
 **  DESCRIPTION: The test_sshd() function builds into the
 **  laus_test framework to verify that the Linux Audit System
 **  accurately logs PAM program audit information for the
 **  "sshd" program.
 **  
 **  This function performs a series of tests on the sshd PAM
 **  program.  Each subtest has its own setup, execution, and cleanup.
 **  The setup may consist of testing for the existence or non-existence
 **  of entities, the dynamic creation of entities, as well as the 
 **  definition of the expected text to be found in the audit.
 **  Each subtest is documented below to describe its own particulars.
 **
 **  HISTORY    :
 **    08/2003 Originated by Dustin Kirkland <k1rkland@us.ibm.com>
 **
 **********************************************************************/

#include "pam_laus.h"
#include "tempname.h"
#include <time.h>
#include <pwd.h>
#include <sys/utsname.h>

int test_sshd(laus_data* dataPtr) {

  int rc = 0;
  int test = 1;
  char* user;
  char* home;
  int uid;
  char* command;
  char* filename;
  int fd;
  int file_exists = 0;

  // Produced with crypt( "eal", "42" ): 42VmxaOByKwlA
  char* password = "eal";
  char* encryptedpassword = "42VmxaOByKwlA";
  //char* badpassword = "anything_but_eal";
  char* executable = "/usr/sbin/sshd";

  dataPtr->msg_euid = 0;
  dataPtr->msg_egid = 0;
  if (( rc = createTempUserName( &user, &uid, &home ) == -1 )) {
    printf1("Out of temp user names\n");
    goto EXIT;
  }
  // Create user
  command = mysprintf( "/usr/sbin/useradd -u %d -d %s -m -g trusted -G trusted -p %s %s", uid, home, encryptedpassword, user );
  if( ( rc = system( command ) ) == -1 ) {
    printf1( "Error creating user [%s]\n", user );
    goto EXIT;
  }
  free( command );
  // Modify configuration files
  // MH: This isn't the test's job.
//  backupFile("/etc/pam.d/sshd");
//  file_exists = backupFile("/etc/ssh/ssh_known_hosts");
  // Configure pam.d
//  if ( ( rc = system( "cat /etc/pam.d/sshd | grep -v laus > /etc/pam.d/sshd; echo \"account required pam_laus.so detach\" >> /etc/pam.d/sshd") ) == -1 ) {
//    printf1( "Error modifying /etc/pam.d/sshd\n" );
//    goto EXIT;
//  }
  // Add localhost to ssh_known_hosts
  if ( ( rc = system( "cat /etc/ssh/ssh_host_rsa_key.pub | awk -F\" \" '{print \"localhost \" $1 \" \" $2}' > /etc/ssh/ssh_known_hosts") ) == -1 ) {
    printf1( "Error modifying /etc/ssh/ssh_known_hosts\n" );
    goto EXIT;
  }

  // Test 1: SUCCESS sshd
  // When a user successfully uses the sshd program.
  // The record is generated with the following commands:
  // sshd
  //
  //
 //TEST_1:
  printf5("TEST %d\n", test++);
  // Setup
  // Create expect script file to execute ssh session
  filename = (char *) malloc(strlen(tempname));
  strcpy(filename, tempname);
  fd = mkstemp(filename);
  command = mysprintf( "expect -c \"spawn /usr/bin/ssh %s@localhost \nsleep 1 \nexpect -re \\\"password: \\\" \nsleep 1 \nsend \\\"%s\\r\\n\\\" \nsleep 1 \nexpect -re \\\"> \\\" \nsleep 1 \nsend \\\"exit\\\" \nsend_user \\\"exit\\n\\\"\"", user, password);
  write(fd, command, strlen(command));
  close(fd);
  fchmod(fd, S_IRWXU | S_IRWXG | S_IRWXO);
  free( command );

  // Execution
  command = mysprintf( "/bin/sh %s", filename ); 
  dataPtr->msg_pid = NO_FORK;
  runPAMProgram( dataPtr, command );
  free( command );

  // Set expected data for audit record
  // uid/gid's are DONT CARES
  dataPtr->msg_login_uid = dataPtr->msg_euid = dataPtr->msg_suid = dataPtr->msg_ruid = dataPtr->msg_fsuid = NO_ID_CHECK;
  dataPtr->msg_login_uid = dataPtr->msg_egid = dataPtr->msg_sgid = dataPtr->msg_rgid = dataPtr->msg_fsgid = NO_ID_CHECK;
  dataPtr->laus_var_data.loginData.uid = uid;
  strcpy(dataPtr->laus_var_data.loginData.hostname, "localhost");
  strcpy(dataPtr->laus_var_data.loginData.address, "127.0.0.1");
  strcpy(dataPtr->laus_var_data.loginData.terminal, "ssh");
  strcpy(dataPtr->laus_var_data.loginData.executable, executable);


  // Check for audit record
  verifyPAMProgram( dataPtr );

  // Cleanup
  unlink( filename );
  free( filename );

  // End Test 1


 EXIT:
  restoreFile("/etc/pam.d/sshd");
  if ( file_exists == -1 ) {
    unlink("/etc/ssh/ssh_known_hosts");
  } else {
    restoreFile("/etc/ssh/ssh_known_hosts");
  }
  command = mysprintf( "/usr/sbin/userdel -r %s", user );
  if( ( rc = system( command ) ) == -1 ) {
    printf1( "Error deleting user [%s]\n", user );
  }
  free( command );
  printf5("Returning from test_sshd()\n");
  return rc;
}


