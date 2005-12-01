
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

#include "includes.h"
#include "libpam.h"
#include "../include/context.h"
#include "tempname.h"
#include <time.h>
#include <pwd.h>

int test_sshd(struct audit_data* dataPtr) {

  int rc = 0;
  int test = 1;
  char* user;
  char* home;
  int uid;
  char* command;
  char* filename;
  int fd;
  int file_exists = 0;
  //time_t t;

  // Produced with crypt( "eal", "42" ): 42VmxaOByKwlA
  char* password = "eal";
  char* encryptedpassword = "42VmxaOByKwlA";
  char* badpassword = "anything_but_eal";


  dataPtr->euid = 0;
  dataPtr->egid = 0;

  if (( rc = createTempUserName( &user, &uid, &home ) == -1 )) {
    printf("Out of temp user names\n");
    goto EXIT;
  }

  // Create user
  command = mysprintf( "/usr/sbin/useradd -u %d -d %s -m -g wheel -G wheel -p %s %s", uid, home, encryptedpassword, user );
  if( ( rc = system( command ) ) == -1 ) {
    printf( "Error creating user [%s]\n", user );
    goto EXIT;
  }
  free( command );
  // Modify configuration files
  file_exists = backupFile("/etc/ssh/ssh_known_hosts");
  if ( ( rc = system( "cat /etc/ssh/ssh_host_rsa_key.pub | awk -F\" \" '{print \"localhost \" $1 \" \" $2}' > /etc/ssh/ssh_known_hosts") ) == -1 ) {
    printf( "Error modifying /etc/ssh/ssh_known_hosts\n" );
    goto EXIT;
  }

  // Test 1: SUCCESS sshd
  // When a user successfully uses the sshd program.
  // The record is generated with the following commands:
  // sshd
  //
  // In addition to the standard audit information, the following string will be logged:
  // PAM authentication: user=%s (hostname=localhost, addr=127.0.0.1, terminal=ssh)
  // PAM accounting: user=%s (hostname=localhost, addr=127.0.0.1, terminal=ssh)
  //
  //
 //TEST_1:
  printf("TEST %d\n", test++);
  // Setup
  // Create expect script file to execute ssh session
  filename = (char *) malloc(strlen(tempname));
  strcpy(filename, tempname);
  fd = mkstemp(filename);
  command = mysprintf( "expect -c \"spawn /usr/bin/ssh %s@localhost \n"
"sleep 1 \n"
"expect -re \\\"password: \\\" \n"
"sleep 1 \n"
"send \\\"%s\\r\\n\\\" \n"
"sleep 1 \n"
"expect -re \\\"> \\\" \n"
"sleep 1 \n"
"send \\\"exit\\\" \n"
"send_user \\\"exit\\n\\\"\"", user, password);
  write(fd, command, strlen(command));
  close(fd);
  fchmod(fd, S_IRWXU | S_IRWXG | S_IRWXO);
  free( command );

  // Execution
  command = mysprintf( "/bin/sh %s", filename ); 
  dataPtr->euid = dataPtr->suid = dataPtr->uid = dataPtr->fsuid = 0;
  dataPtr->egid = dataPtr->sgid = dataPtr->gid = dataPtr->fsgid = 0;
  dataPtr->pid = NO_FORK;
  runPAMProgram( dataPtr, command );
  free( command );

  // Check for audit record

  // uid/gid's are DONT CARES for the libpam tests, luid not yet set
  dataPtr->loginuid = dataPtr->euid = dataPtr->suid = dataPtr->uid = dataPtr->fsuid = NO_ID_CHECK;
  dataPtr->loginuid = dataPtr->egid = dataPtr->sgid = dataPtr->gid = dataPtr->fsgid = NO_ID_CHECK;
  dataPtr->type = AUDIT_MSG_USER;

  //strncpy(dataPtr->msg_evname, "AUTH_success", sizeof(dataPtr->msg_evname));
  dataPtr->comm = mysprintf("PAM setcred: user=%s exe=./usr/sbin/sshd.*terminal=ssh result=Success", user);
  verifyPAMProgram( dataPtr );

  //strncpy(dataPtr->msg_evname, "AUTH_success", sizeof(dataPtr->msg_evname));
  dataPtr->comm = mysprintf("PAM accounting: user=%s exe=./usr/sbin/sshd.*terminal=ssh result=Success", user);
  verifyPAMProgram( dataPtr );

  // Cleanup
  unlink( filename );
  free( filename );
  free( dataPtr->comm );

  // End Test 1



  // Test 2: FAILURE sshd
  // When a user unsuccessfully uses the sshd program.
  // The record is generated with the following commands:
  // ssh (and a bad password)
  //
  // In addition to the standard audit information, the following string will be logged:
  // PAM authentication: user=%s (hostname=localhost, addr=127.0.0.1, terminal=ssh)
  //
  //
 //TEST_2:
  printf("TEST %d\n", test++);
  // Setup
  // Create expect script file to execute ssh session
  filename = (char *) malloc(strlen(tempname));
  strcpy(filename, tempname);
  fd = mkstemp(filename);
  command = mysprintf( "expect -c \"spawn /usr/bin/ssh %s@localhost \n"
"sleep 1 \n"
"expect -re \\\"password: \\\" \n"
"sleep 1 \n"
"send \\\"%s\\r\\n\\\" \n"
"sleep 1 \n"
"expect -re \\\"> \\\" \n"
"sleep 1 \n"
"send \\\"exit\\\" \n"
"send_user \\\"exit\\n\\\"\"", user, badpassword);
  write(fd, command, strlen(command));
  close(fd);
  fchmod(fd, S_IRWXU | S_IRWXG | S_IRWXO);
  free( command );

  // Execution
  command = mysprintf( "/bin/sh %s", filename );
  dataPtr->euid = dataPtr->suid = dataPtr->uid = dataPtr->fsuid = 0;
  dataPtr->egid = dataPtr->sgid = dataPtr->gid = dataPtr->fsgid = 0;
  dataPtr->pid = NO_FORK;
  runPAMProgram( dataPtr, command );
  free( command );

  // Check for audit record

  // uid/gid's are DONT CARES for the libpam tests
  dataPtr->loginuid = dataPtr->euid = dataPtr->suid = dataPtr->uid = dataPtr->fsuid = NO_ID_CHECK;
  dataPtr->loginuid = dataPtr->egid = dataPtr->sgid = dataPtr->gid = dataPtr->fsgid = NO_ID_CHECK;

  
  //strncpy(dataPtr->msg_evname, "AUTH_failure", sizeof(dataPtr->msg_evname));
  dataPtr->comm = mysprintf("PAM authentication: user=%s exe=./usr/sbin/sshd.*terminal=ssh result=Authentication failure", user);
  verifyPAMProgram( dataPtr );

  // Cleanup
  unlink( filename );
  free( filename );
  free( dataPtr->comm );

  // End Test 2



 EXIT:
  printf("exit: in test_sshd()\n");
  if ( file_exists == -1 ) {
    unlink("/etc/ssh/ssh_known_hosts");
  } else {
    restoreFile("/etc/ssh/ssh_known_hosts");
  }
  restoreFile("/etc/login.defs");
  command = mysprintf( "/usr/sbin/userdel -r %s", user );
  if( ( rc = system( command ) ) == -1 ) {
    printf( "Error deleting user [%s]\n", user );
  }
  free( command );
  printf("Returning from test_sshd()\n");
  return rc;
}


