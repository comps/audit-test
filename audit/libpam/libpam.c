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
**  FILE   : libpam.c
**
**  PURPOSE: This file contains the main body of the Linux Auditing 
**           System test suite for Pluggable Authentication Modules.  
**           
**           This file, as well as 
**           the accompanying headers in the ../includes/ directory and the
**           helper utility functions in the ../utils/ directory, form a
**           framework that facilitates a robustly modular test suite.
**
**           Each test is written as an independent function to be
**           called by the main body in this file.  This design 
**           conveniently allows multiple developers to implement
**           test functions for separate pam programs (located in the
**           ./tests/ directory) and these functions simply plug into
**           this framework.  
**           
**           As the tests become available, pointers to these
**           functions are added to the pamTests[] array.  Each
**           function in this array is called to execute the tests
**           contained therein.  Documentation for each test can be
**           found in the source code prolog of each test file.
**
**           Aside from the test functions themselves, there are several
**           helper utility functions that are used.  Documentation for
**           each utility can be found in the source code prolog of each
**           utility file.
**          
**
**  HISTORY:
**    08/2003 originated by Dustin Kirkland (k1rkland@us.ibm.com)
**
**********************************************************************/

#include "includes.h"
#include "libpam.h"
#include <pwd.h>
#include <getopt.h>

int pass_testcases = 0;
int fail_testcases = 0;
int skip_testcases = 0;
int debug = 2;
uid_t login_uid = 0;
char cwd[PATH_MAX];

/*
** main
*/
int main(int argc, char **argv)
{
  int test_rc = 0;
  int rc    = 0;
  int k     = 0;
  uid_t uid = 0;
  gid_t gid = 0;
  char* testcase = NULL;
  char* defaultUser = "nobody";
  char* usage = "Usage:\n\
  laustest [-u $TESTUSER] [-t $TRUSTEDPROGRAM] [-d $DEBUG]\n\n\
Arguments:\n\
  -u $TESTUSER:  Specify the user to run the tests as $TESTUSER.\n\
                 This user must actually exist on the system.\n\
                 By default, this is 'nobody'\n\
  -t $PAM:       Optionally specify a single test case to run.\n\
                 $PAM is the name of the pam program call to test.\n\
                 By default, all test cases are executed.\n\
  -d $DEBUG:     Optionally specify the debug level 0-5.\n\
                 0  No messages\n\
                 1  + Environment messages\n\
                 2  + Test messages (Default)\n\
                 3  + Warning messages\n\
                 4  + Info messages\n\
                 5  + Full debug messages\n\n";


  struct passwd* passwd_data = NULL;
  struct passwd* login_uid_data = NULL;
  audit_data* successDataPtr = NULL;

#if defined(__i386__)
	int arch = AUDIT_ARCH_I386;
#elif defined(__powerpc64__)
	int arch = AUDIT_ARCH_PPC64;
#elif defined(__powerpc__)
	int arch = AUDIT_ARCH_PPC;
#elif defined(__s390x__)
        int arch = AUDIT_ARCH_S390X;
#elif defined(__s390__)
	int arch = AUDIT_ARCH_S390;
#elif defined(__x86_64__)
	int arch = AUDIT_ARCH_X86_64;
#elif defined(__ia64__)
	int arch = AUDIT_ARCH_IA64;
#endif

  // Array contains data for each pam program test.
  pam_data pamTests[] = {
    /* fnptr, testname, dataptr */
    {&test_login, "login", NULL},
    {&test_sshd, "sshd", NULL},
    {&test_su, "su", NULL},
    {&test_vsftpd, "vsftpd", NULL}
  };

  //backupFile("/etc/audit/filter.conf");

  // Iterate through command line options
  while (1) {
    int option_index = 0;
    // BUGBUG: Not sure if this is actually necessary
    static struct option long_options[] = {
      {"user", 1, 0, 0},
      {"testcase", 1, 0, 0},
      {"debug", 1, 0, 0},
      {"login", 1, 0, 0},
      {0, 0, 0, 0}
    };
    int c = getopt_long (argc, argv, "u:t:d:l:",
                         long_options, &option_index);
    if (c == -1) break;
    switch (c) {
      case 'u':
        // User option specified, try to get passwd info, exit if none
        if ((passwd_data = getpwnam(optarg)) == NULL) {
          printf("ERROR: Unable to get %s passwd info.\n", optarg);
          goto EXIT_ERROR;
        }
        break;
      case 'l':
        // login uid specified, try to get passwd info, exit if none
        if ((login_uid_data = getpwnam(optarg)) == NULL) {
          printf("ERROR: Unable to get %s passwd info.\n", optarg);
          goto EXIT_ERROR;
        }
	login_uid = login_uid_data->pw_uid;
        break;
      case 't':
        // Individual test case specified, exit if invalid value
        testcase = (char *) malloc(strlen(optarg) + 1);	
        sprintf(testcase, "%s", optarg);				// optarg is not null-terminated
        for (k = 0; k < sizeof(pamTests)/sizeof(pam_data); k++) {
          if ((rc = strcmp(testcase, pamTests[k].testName)) == 0 ) {
            break;
          }
        }
        if ( rc != 0 ) {
          printf("ERROR: System call '%s' not tested by laustest\n", testcase);
          goto EXIT_ERROR;
        }
        break;
      case 'd':
        // Debug level specified
        debug = atoi(optarg);
        // BUGBUG: This if statement needs to be modified to handle
        // non-numeric input correctly.
        if ( (debug < 0) || (debug > 9) ) {
          printf("ERROR: Debug level %s is invalid\n", optarg);
          goto EXIT_ERROR;
        }
        break;
      default :
        // Help option or unhandled option specified, display usage text and exit
        printf("%s", usage);
        rc = 1;
        goto EXIT_HELP;
        break;
    }
  }

  if (passwd_data == NULL) {
    // If no user specified, try defaultUser
    passwd_data = getpwnam(defaultUser);
  }

  if (passwd_data == NULL) {
    // Must enter a valid user to run tests as
    printf("ERROR: Please enter a test user name with the -u option.\n");
    goto EXIT_ERROR;
  }

  if (login_uid_data == NULL) {
    // Get login uid from system
    login_uid = getLoginUID();
  }

  uid = passwd_data->pw_uid;
  gid = passwd_data->pw_gid;

  // Save the CWD for audit_set_filters()
  getcwd(cwd, PATH_MAX);

  /*
  ** For these tests, we want to log all success and failures
  if ( ( rc = system("echo \"event user-message = always;\nevent process-login = always;\" > /etc/audit/filter.conf") ) != 0 ) {
    printf("Could not configure filter.conf\n");
    goto EXIT_ERROR;
  }
  */

  if ((rc = audit_reload()) != 0) {
    goto EXIT_ERROR;
  }

  /*
  ** Loop through pam programs
  **
  */
  successDataPtr = (audit_data*)malloc(sizeof(audit_data));

  for (k = 0; k < sizeof(pamTests)/sizeof(pam_data); k++) {
    // Run exactly one test case?
    if ((testcase != NULL) && 
	(strcmp(testcase, pamTests[k].testName) != 0 )) {
      continue;
    }

    printf("%s()\n", pamTests[k].testName);

    memset(successDataPtr,'\0',sizeof(audit_data));

    successDataPtr->success = 1;
    successDataPtr->u.syscall.arch = arch;
    successDataPtr->type = AUDIT_MSG_LOGIN;
    successDataPtr->euid = uid;
    successDataPtr->egid = gid;
    successDataPtr->testName = pamTests[k].testName;
    
    printf("Performing test on %s\n", pamTests[k].testName);

    if ((rc = pamTests[k].testPtr(successDataPtr)) != 0) {
      goto EXIT_ERROR;
    }
    
  }

  free(successDataPtr);

  if (test_rc != 0) {
    printf("ERROR: At least 1 pam program did not execute as expected\n");
    rc = -1;
  }

  printf("PASSED = %i, FAILED = %i\n", pass_testcases, fail_testcases);

 //XXXX
  //restoreFile("/etc/audit/filter.conf");
  audit_reload();

EXIT_HELP:
  if (testcase != NULL)
    free (testcase);
  return rc;

EXIT_ERROR:
//  restoreFile("/etc/audit/filter.conf");
  audit_reload();
  printf("ERROR: Test aborted: errno = %i\n", errno);
  return rc;

}
