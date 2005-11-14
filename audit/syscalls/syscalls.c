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

#include <libaudit.h>
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
static int verify(int, struct audit_data *, log_options, 
		  struct test_counters *);

/* global to utils lib */
int debug = 2;
uid_t login_uid = 0;

/* global to syscalls tests */
char cwd[PATH_MAX];
int helper_uid;

int main(int argc, char **argv)
{
    int rc = 0;				// main return code
    struct syscall_opts options;	// CLI options
    int logOptionsIndex = 0;
    int i;				// loop counter
    Boolean success;			// loop counter
    char test_user[LOGIN_NAME_MAX + 1];	// test user
    struct passwd *test_user_pw = NULL;	// test user's /etc/passwd data
    struct audit_data context;		// expected log data
    int test_rc;			// testcase return code
    struct test_counters count = { 0, 0, 0 }; // aggregate results
    char *command = NULL;		// command string
    int sysnum;				// syscall number

    char *helper;
    char *helper_homedir;

    /* 
     * Parse and check command line options
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
	for (i = 0; i < sizeof(syscallTests) / sizeof(syscall_data); i++) {
	    if ((rc = strcmp(options.testcase,
			     syscallTests[i].testName)) == 0) {
		break;
	    }
	}
	if (rc != 0) {
	    printf1("ERROR: System call '%s' not tested by laustest\n", 
		    options.testcase);
	    goto EXIT;
	}
    }

    if (strcmp(options.user, "") != 0) {
	strcpy(test_user, options.user);
    } else {
	strcpy(test_user, DEFAULT_TEST_USER);
    }
    if ((test_user_pw = getpwnam(test_user)) == NULL) {
	printf1("ERROR: Unable to get passwd info for [%s]\n", test_user);
	rc = -1;
	goto EXIT;
    }

    /*
     * Get system info
     */
    if (getcwd(cwd, PATH_MAX) == NULL) {
	printf1("ERROR: Unable to get current working directory\n");
	rc = -1;
	goto EXIT;
    }

    if ((login_uid = getLoginUID()) == -1) {
	printf1("ERROR: Unable to get login uid\n");
	rc = -1;
	goto EXIT;
    }

    if ((rc = createTempUserName(&helper, &helper_uid, &helper_homedir)) == -1) {
	printf1("Out of temp user names\n");
	goto EXIT;
    }
    command = mysprintf("/usr/sbin/useradd -u %d -d %s -m %s",
			helper_uid, helper_homedir, helper);
    if ((rc = system(command)) == -1) {
	printf1("Error creating user [%s]\n", helper);
	goto EXIT;
    }
    free(command);

    /*
     * Initialize audit subsystem
     *
     * XXX stopping and clearing the audit trail seems to detach this
     * process from the audit daemon in the 2.6 LAuS implementation
     */

    /*
     * Variations on logging directives
     * - both sucess and failure (fast mode)
     * - success only
     * - failure only
     * - none
     */
    for (logOptionsIndex = 0;
	 logOptionsIndex < sizeof(logOptions) / sizeof(log_options);
	 logOptionsIndex++) {

	if (options.fast_mode && logOptionsIndex > 0)
	    break;

	if ((rc = audit_set_filters(logOptions[logOptionsIndex])) == -1) {
	    printf1("ERROR: Test aborted: errno = %i\n", errno);
	    goto EXIT_CLEANUP;
	}

	/*
	 * System call testcases
	 */
	for (i = 0; i < sizeof(syscallTests) / sizeof(syscall_data); i++) {

	    if ((strcmp(options.testcase, "") != 0) &&
		(strcmp(options.testcase, syscallTests[i].testName) != 0)) {
		continue;
	    }

	    /*
	     * Sucess, failure testcases
	     */
	    success = TRUE;
	    while ((success == TRUE) || (success == FALSE)) {
		/*
		 * Initialize expected record
		 */
		memset(&context, '\0', sizeof(struct audit_data));
		context.type = AUDIT_MSG_SYSCALL;
		context.euid = test_user_pw->pw_uid;
		context.egid = test_user_pw->pw_gid;
		context.fsuid = test_user_pw->pw_uid;
		context.fsgid = test_user_pw->pw_gid;
		context.success = success;
		context.u.syscall.arch = TS_BUILD_ARCH;
		sysnum = audit_name_to_syscall(syscallTests[i].testName,
					       audit_detect_machine());
		if (sysnum == -1) {
		    printf1(
			"ERROR: Could not translate syscall number for: %s\n", 
			    syscallTests[i].testName);
		    goto EXIT_CLEANUP;
		}
		context.u.syscall.sysnum = sysnum;

		/*
		 * Perform test
		 */
		printf2(
		    "Performing test on %s() [logSuccess=%d, logFailure=%d, successCase=%d]\n",
		    syscallTests[i].testName,
		    logOptions[logOptionsIndex].logSuccess,
		    logOptions[logOptionsIndex].logFailure,
		    context.success);
		test_rc = syscallTests[i].testPtr(&context);

		/*
		 * Determine results
		 */
		verify(test_rc, &context, logOptions[logOptionsIndex], &count);

		/* 
		 * u.syscall.data is allocated during each test run,
		 * and must be freed per iteration.
		 */
		if (context.u.syscall.args) {
		    free(context.u.syscall.args);
		}

		success--;
	    }
	}
    }
    printf2("PASSED = %i, FAILED = %i, SKIPPED = %i\n",
	    count.passed, count.failed, count.skipped);

EXIT_CLEANUP:
    /* System cleanup */
    command = mysprintf("/usr/sbin/userdel -r %s", helper);
    if ((rc = system(command)) == -1) {
	printf1("Error deleting user [%s]\n", helper);
    }
    free(command);

    /* Audit cleanup */
    audit_reload();

EXIT:
    return rc;
}

static
int verify(int return_code, struct audit_data *context, log_options logOption,
	   struct test_counters *count)
{
    int rc = 0;

    printf4("verify(return = %d)\n", return_code);

    switch (return_code) {
	case 0:
	    rc = audit_verify_log(context, logOption);
	    if (rc < 0) {
		goto EXIT_ERROR;
	    }

	    if ((context->success && logOption.logSuccess) ||
		(!context->success && logOption.logFailure)) {
		printf2("Verify record\n");
		if (rc > 0) {
		    count->passed++;
		    printf2("AUDIT PASS ");
		} else {
		    count->failed++;
		    debug_expected(context);
		    printf2("AUDIT FAIL ");
		}
	    } else {
		printf2("Verify no record\n");
		if (rc == 0) {
		    count->passed++;
		    printf2("AUDIT PASS ");
		} else {
		    count->failed++;
		    printf2("AUDIT FAIL ");
		}
	    }
	    printf2prime
		(": '%s' [logSuccess=%x, logFailure=%x, successCase=%x]\n",
		 "", logOption.logSuccess, logOption.logFailure,
		 context->success);
	    break;

	case SKIP_TEST_CASE:
	    count->skipped++;
	    printf2
		("%s() test case skipped: [logSuccess=%d, logFailure=%d, successCase=%d]\n",
		 "", logOption.logSuccess, logOption.logFailure,
		 context->success);
	    break;

	default:
	    count->failed++;
	    printf1
		("ERROR: %s() test invalid: [logSuccess=%d, logFailure=%d, successCase=%d]\n",
		 "", logOption.logSuccess, logOption.logFailure,
		 context->success);
	    printf2
		("AUDIT FAIL : '%s' test invalid: [%s, logSuccess=%d, logFailure=%d]\n",
		 "", context->success ? "successCase" : "failureCase",
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
