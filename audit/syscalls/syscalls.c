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
**  FILE   : syscalls.c
**
**  PURPOSE: This file contains the main body of the Linux Auditing 
**           System test suite for system calls.  This file, as well as 
**           the accompanying headers in the ./includes/ directory and the
**           helper utility functions in the ./utils/ directory, form a
**           framework that facilitates a robustly modular test suite.
**
**           Each test is written as an independent function to be
**           called by the main body in this file.  This design 
**           conveniently allows multiple developers to implement
**           test functions for separate system calls (located in the
**           ./tests/ directory) and these functions simply plug into
**           this framework.  
**           
**           As the tests become available, pointers to these
**           functions are added to the syscallTests[] array.  Each
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
**    05/03 originated by Dan Jones (danjones@us.ibm.com)
**    06/03 furthered by Dustin Kirkland (k1rkland@us.ibm.com)
**    06/03 furthered by Michael A. Halcrow <mike@halcrow.us>
**
**********************************************************************/

#include "includes.h"
#include "syscalls.h"
#include "syscalls_array.h"	// global syscallTests[] array

#include "logoptions.h"

#include <pwd.h>

struct syscall_opts {
    char user[LOGIN_NAME_MAX + 1];
    char testcase[TESTCASE_NAME_MAX + 1];
    int debug_mode;
    int fast_mode;
    int help;
};

static int parse_command_line(int, char **, struct syscall_opts *);
static void usage();
static int verify(int, laus_data *, log_options);

char cwd[PATH_MAX];

int pass_testcases = 0;
int fail_testcases = 0;
int skip_testcases = 0;
int logOptionsIndex = 0;
int debug = 2;
uid_t login_uid = 0;
char *helper;
int helper_uid;
char *helper_homedir;

int main(int argc, char **argv)
{
    int rc = 0;
    struct syscall_opts options;
    int j, k = 0;
    uid_t uid = 0;
    gid_t gid = 0;
    laus_data test_data;
    int test_rc;
    Boolean success;
    char *command;
    char test_user[LOGIN_NAME_MAX + 1];
    struct passwd *passwd_data = NULL;

#ifdef __IX86
    int arch = AUDIT_ARCH_I386;
#endif
#ifdef __PPC32
    int arch = AUDIT_ARCH_PPC;
#endif
#ifdef __PPC64
    int arch = AUDIT_ARCH_PPC64;
#endif
#ifdef __S390X
    int arch = AUDIT_ARCH_S390X;
#endif
#ifdef __S390
    int arch = AUDIT_ARCH_S390;
#endif
#ifdef __X86_64
    int arch = AUDIT_ARCH_X86_64;
#endif
#ifdef __IA64
    int arch = AUDIT_ARCH_IA64;
#endif

    /* Initialize global structures */
    init_globals();

    // Clear the audit trail
// BUGBUG: Stopping and clearing the audit trail seems to detach this process
//   from the audit daemon in the 2.6 LAuS implementation

    // Create helper user
    if ((rc = createTempUserName(&helper, &helper_uid, &helper_homedir)) == -1) {
	printf1("Out of temp user names\n");
	goto EXIT;
    }
    // Create user
    command =
	mysprintf("/usr/sbin/useradd -u %d -d %s -m %s", helper_uid,
		  helper_homedir, helper);
    if ((rc = system(command)) == -1) {
	printf1("Error creating user [%s]\n", helper);
	goto EXIT;
    }
    free(command);

    /* 
     * Parse & check command line options
     */
    rc = parse_command_line(argc, argv, &options);
    if ((rc == -1) || (options.help)) {
	usage();
	goto EXIT;
    }

    if (options.debug_mode != -1) {
	debug = options.debug_mode;
    }

    if (strcmp(options.testcase,"") != 0) {
	for (k = 0; k < sizeof(syscallTests) / sizeof(syscall_data); k++) {
	    if ((rc = strcmp(options.testcase,
			     syscallTests[k].testName)) == 0) {
		break;
	    }
	}
	if (rc != 0) {
	    printf1("ERROR: System call '%s' not tested by laustest\n", 
		    options.testcase);
	    goto EXIT_ERROR;
	}
    }

    if (strcmp(options.user, "") != 0) {
	strcpy(test_user, options.user);
    } else {
	strcpy(test_user, DEFAULT_TEST_USER);
    }
    if ((passwd_data = getpwnam(test_user)) == NULL) {
	printf1("ERROR: Unable to get passwd info for [%s]\n", test_user);
	goto EXIT_ERROR;
    }
    uid = passwd_data->pw_uid;
    gid = passwd_data->pw_gid;

    login_uid = getLoginUID();
    if (login_uid == -1) {
	printf1("ERROR: Unable to get login uid\n");
	goto EXIT_ERROR;
    }

    // Save the CWD for those tests that require it (i.e., test_execve)
    getcwd(cwd, PATH_MAX);

    //backup the filter.conf file
#ifdef CONFIG_AUDIT_LAUS
    backupFile("/etc/audit/filter.conf");
#endif

    // This loop tests all the combinations of logSuccess
    // and logFailure. It is the only type of domain filtering
    // performed within this set of tests.
    for (logOptionsIndex = 0;
	 logOptionsIndex < sizeof(logOptions) / sizeof(log_options);
	 logOptionsIndex++) {

	if (options.fast_mode && logOptionsIndex > 0)
	    break;

	// Set the filter domain (logSuccess, logFailure, both, none)
	if ((rc = audit_set_filters(logOptions[logOptionsIndex])) == -1) {
	    goto EXIT_ERROR;
	}
	// Loop through syscalls
	for (j = 0; j < sizeof(syscallTests) / sizeof(syscall_data); j++) {

	    if ((strcmp(options.testcase, "") != 0) &&
		(strcmp(options.testcase, syscallTests[j].testName) != 0)) {
		continue;
	    }

	    success = TRUE;
	    while ((success == TRUE) || (success == FALSE)) {

		/* initialize test data per iteration */
		memset(&test_data, '\0', sizeof(laus_data));
		test_data.msg_arch = arch;
		test_data.msg_type = AUDIT_MSG_SYSCALL;
		test_data.msg_euid = uid;
		test_data.msg_egid = gid;
		test_data.msg_fsuid = uid;
		test_data.msg_fsgid = gid;
		test_data.testName = syscallTests[j].testName;
		test_data.successCase = success;

		printf2(
		    "Performing test on %s() [logSuccess=%d, logFailure=%d, successCase=%d]\n",
		    syscallTests[j].testName,
		    logOptions[logOptionsIndex].logSuccess,
		    logOptions[logOptionsIndex].logFailure,
		    test_data.successCase);
		test_rc = syscallTests[j].testPtr(&test_data);
		verify(test_rc, &test_data, logOptions[logOptionsIndex]);
		/* 
		 * syscallData.data is allocated during each test run,
		 * and must be freed per iteration.
		 */
		if (test_data.laus_var_data.syscallData.data) {
		    free(test_data.laus_var_data.syscallData.data);
		}

		success--;
	    }
	}
    }
    printf2("PASSED = %i, FAILED = %i, SKIPPED = %i\n",
	    pass_testcases, fail_testcases, skip_testcases);
    goto EXIT;


EXIT_ERROR:
    printf1("ERROR: Test aborted: errno = %i\n", errno);

EXIT:
    command = mysprintf("/usr/sbin/userdel -r %s", helper);
    if ((rc = system(command)) == -1) {
	printf1("Error deleting user [%s]\n", helper);
    }
    free(command);

#ifdef CONFIG_AUDIT_LAUS
    restoreFile("/etc/audit/filter.conf");
#endif
    audit_reload();

    return rc;

}

static
int verify(int return_code, laus_data *dataPtr, log_options logOption)
{
    int rc = 0;

    printf4("verify(return = %d)\n", return_code);

    switch (return_code) {
	case 0:
	    rc = audit_verify_log(dataPtr, logOption);
	    if (rc < 0) {
		goto EXIT_ERROR;
	    }

	    if ((dataPtr->successCase && logOption.logSuccess) ||
		(!dataPtr->successCase && logOption.logFailure)) {
		printf2("Verify record\n");
		if (rc > 0) {
		    pass_testcases++;
		    printf2("AUDIT PASS ");
		} else {
		    fail_testcases++;
		    debug_expected(dataPtr);
		    printf2("AUDIT FAIL ");
		}
	    } else {
		printf2("Verify no record\n");
		if (rc == 0) {
		    pass_testcases++;
		    printf2("AUDIT PASS ");
		} else {
		    fail_testcases++;
		    printf2("AUDIT FAIL ");
		}
	    }
	    printf2prime
		(": '%s' [logSuccess=%x, logFailure=%x, successCase=%x]\n",
		 dataPtr->testName, logOption.logSuccess, logOption.logFailure,
		 dataPtr->successCase);
	    break;

	case SKIP_TEST_CASE:
	    skip_testcases++;
	    printf2
		("%s() test case skipped: [logSuccess=%d, logFailure=%d, successCase=%d]\n",
		 dataPtr->testName, logOption.logSuccess, logOption.logFailure,
		 dataPtr->successCase);
	    break;

	default:
	    fail_testcases++;
	    printf1
		("ERROR: %s() test invalid: [logSuccess=%d, logFailure=%d, successCase=%d]\n",
		 dataPtr->testName, logOption.logSuccess, logOption.logFailure,
		 dataPtr->successCase);
	    printf2
		("AUDIT FAIL : '%s' test invalid: [%s, logSuccess=%d, logFailure=%d]\n",
		 dataPtr->testName,
		 dataPtr->successCase ? "successCase" : "failureCase",
		 logOption.logSuccess, logOption.logFailure);
	    break;
    }

EXIT_ERROR:
    return rc;
}

static int parse_command_line(int argc, char **argv, 
			      struct syscall_opts *options)
{
    int opt;
    int rc = 0;

    /* option defaults */
    strcpy(options->user, "");
    strcpy(options->testcase, "");
    options->debug_mode = -1;
    options->fast_mode = 0;
    options->help = 0;

    while ((opt = getopt(argc, argv, "u:t:d:fh")) != -1) {
	switch(opt) {
	    case 'u':
		strncpy(options->user, optarg, LOGIN_NAME_MAX);
		break;
		break;
	    case 't':
		strncpy(options->testcase, optarg, TESTCASE_NAME_MAX);
		break;
	    case 'd':
		if ((strlen(optarg) > 1) ||
		    (optarg[0] < '0') ||
		    (optarg[0] > '9')) {
		    rc = -1;
		} else {
		    options->debug_mode = atoi(optarg);
		}
		break;
	    case 'f':
		options->fast_mode = 1;
		break;
	    case 'h':
		options->help = 1;
		break;
	    default:
		rc = -1;
	}
    }

    return rc;
}

static void usage()
{
    char *usage = "Usage:\n\
  laustest [-u $TESTUSER] [-t $SYSCALL] [-d $DEBUG]\n\n\
Arguments:\n\
  -u $TESTUSER:  Specify the user to run the tests as $TESTUSER.\n\
                 This user must actually exist on the system.\n\
                 By default, this is 'nobody'\n\
  -t $SYSCALL:   Optionally specify a single test case to run.\n\
                 $SYSCALL is the name of the system call to test.\n\
                 By default, all test cases are executed.\n\
  -f             Fast mode, test (logSuccess=1, logFailure=1) only\n\
  -d $DEBUG:     Optionally specify the debug level 0-5.\n\
                 0  No messages\n\
                 1  + Environment messages\n\
                 2  + Test messages (Default)\n\
                 3  + Warning messages\n\
                 4  + Info messages\n\
                 5  + Full debug messages\n\n";

    printf("%s\n", usage);
}
