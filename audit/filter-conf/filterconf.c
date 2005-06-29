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
**  FILE   : filterconf.c
**
**  PURPOSE: This file contains the main body of the Linux Auditing
**           System test suite for the filter configuration test.
**           This file, as well as the accompanying headers in the
**           ./includes/ directory and the helper utility functions in
**           the ./utils/ directory, form a framework that facilitates a
**           robustly modular test suite.
**
**           Each test is written as an independent function to be
**           called by the main body in this file.  This design
**           conveniently allows multiple developers to implement
**           test functions for separate filter tests (located in the
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
**    08/03 originated by Tom Lendacky (toml@us.ibm.com)
**
**********************************************************************/

#include "includes.h"

#include "filterconf.h"
#include "filterconf_array.h"
#include "set_config.h"

#include <pwd.h>
#include <getopt.h>


int pass_testcases = 0;
int fail_testcases = 0;
int skip_testcases = 0;

int debug = 2;

uid_t login_uid = 0;

char cwd[PATH_MAX];

char test_devname[DEVNAME_LEN];

char test_exe[PATH_MAX];


// usage
void usage() {

	printf(
		"usage:\n"
		"filterconf [--user | -u TEST-USER]\n"
		"           [--login | -l LOGIN-USER]\n"
		"           [--testcase | -t TESTCASE-NAME]\n"
		"           [--debug | -d DEBUG-LEVEL]\n"
		"           [--help | -h]\n"
		"  TEST-USER:     The user name to use for the subset of testcases\n"
		"                 that require using a user other than root.\n"
		"                 Default: 'nobody'\n"
		"  LOGIN-USER:    The user name to use to represent the user name that\n"
		"                 was originally specified on the login.\n"
		"  TESTCASE-NAME: The name of a particular testcase to run (enclosed\n"
		"                 in quotes if the name contains spaces).\n"
		"  DEBUG-LEVEL:   The level of debug information to display\n"
		"                 Default: 2\n"
		"                   0 - No messages\n"
		"                   1 - Environment messages\n"
		"                   2 - + Test messages\n"
		"                   3 - + Warning messages\n"
		"                   4 - + Info messages\n"
		"                   5 - + Debug messages\n"
		"                   8 - + Audit related messages\n"
		"                   9 - + Audit debug messages\n"
	);
}


// main
int main(int argc, char **argv) {

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

	int i;

	int option;
	char short_opts[] = "d:hl:t:u:";
	struct option long_opts[] = {
			{"debug",    1, 0, 'd'},
			{"help",     0, 0, 'h'},
			{"login",    1, 0, 'l'},
			{"testcase", 1, 0, 't'},
			{"user",     1, 0, 'u'},
			{0, 0, 0, 0},
		};

	int rc = 0;

	char *l_user = NULL;
	struct passwd *l_passwd = NULL;

	char *d_user = "nobody";
	char *u_user = NULL;
	struct passwd *u_passwd = NULL;
	int u_arg = 0;

	char *testcase = NULL;

	laus_data ld;

	init_globals();

	// Iterate through command line options
	while ((option = getopt_long(argc, argv, short_opts, long_opts, NULL)) != -1) {
		switch (option) {
			case 'd':
				// Debug level specified
				debug = atoi(optarg);
				// BUGBUG: This if statement needs to be modified to handle
				// non-numeric input correctly.
				if ((debug < 0) || (debug > 9)) {
					printf1("ERROR: Debug level '%s' is not valid\n", optarg);
					goto EXIT_ERROR;
				}

				break;

			case 'h':
				// Help requested
				usage();
				goto EXIT;

				break;

			case 'l':
				l_user = strdup(optarg);

				break;

			case 't':
				testcase = strdup(optarg);

				break;

			case 'u':
				u_user = strdup(optarg);
				u_arg = 1;

				break;

			default:
				usage();
				goto EXIT_ERROR;

				break;
		}
	}

	// Be sure the test was invoked as root
	if (getuid() != 0) {
		printf1("ERROR: You must be running as 'root' to run this test\n");
		goto EXIT_ERROR;
	}

	// Check for a login user name
	if (l_user) {
		// Try to get passwd info for the login user, exit if none
		if ((l_passwd = getpwnam(l_user)) == NULL) {
			printf1("ERROR: Unable to obtain '%s' passwd info\n", l_user);
			goto EXIT_ERROR;
		}

		login_uid = l_passwd->pw_uid;

		printf5("Using login user name '%s' with login uid '%d'\n", l_user, login_uid);
	} else {
          // Get login uid from system
          login_uid = getLoginUID();
printf("USING LUID = %d\n", login_uid);
        }

	// Try to get passwd info for the test user, exit if none
	if (!u_user)
		u_user = strdup(d_user);
	if ((u_passwd = getpwnam(u_user)) == NULL) {
		printf1("ERROR: Unable to obtain '%s' passwd info\n", u_user);
		printf1("ERROR: Please enter a valid test user name with the '--user' option\n");
		goto EXIT_ERROR;
	}
	printf5("Using test user name '%s' with test uid '%d'\n", u_user, u_passwd->pw_uid);

	// Check for a specific test case run
	if (testcase) {
		// Individual test case specified, exit if invalid value
		for (i = 0; i < (sizeof(filterconf_tests) / sizeof(filterconf_data)); i++) {
			if ((rc = strcmp(testcase, filterconf_tests[i].testName)) == 0 )
				break;
		}
		if (rc) {
			printf1("ERROR: Filterconf test '%s' does not exit\n", testcase);
			goto EXIT_ERROR;
		}
	}

	// Save the executable name for login filter tests
	if (realpath(argv[0], test_exe) == 0) {
		printf1("ERROR: realpath failed - errno=%d %s\n", errno, strerror(errno));
		goto EXIT_ERROR;
	}

	// Initialize the audit configuration
	if (init_audit_conf(u_passwd->pw_uid, u_passwd->pw_gid) < 0) {
		printf1("ERROR: Unable to initialize the audit configuration\n");
		goto EXIT_ERROR;
	}

	// Loop through filterconf tests
	for (i = 0; i < (sizeof(filterconf_tests) / sizeof(filterconf_data)); i++) {
		// Run exactly one test case?
		if (testcase && (strcmp(testcase, filterconf_tests[i].testName) != 0 ))
			continue;

		memset(&ld, 0, sizeof(laus_data));
		ld.successCase = TRUE;
		ld.msg_arch    = arch;
		ld.testName    = filterconf_tests[i].testName;

		if (set_filter_conf(filterconf_tests[i].filter) < 0) {
			printf1("ERROR: Filter configuration setup failed for test '%s'\n", filterconf_tests[i].testName);
			goto EXIT_ERROR;
		}

		printf2("Performing test '%s'\n", filterconf_tests[i].testName);
		rc = filterconf_tests[i].testPtr(&ld, u_passwd->pw_uid, u_passwd->pw_gid);
		if (rc != 0) {
			printf1("ERROR: Test '%s' failed\n", filterconf_tests[i].testName);
			goto EXIT_ERROR;
		}

		audit_verify_log(&ld, filterconf_tests[i].expectedLogResult);

		if ((ld.successCase && 
		     filterconf_tests[i].expectedLogResult.logSuccess) ||
		    (!ld.successCase && 
		     filterconf_tests[i].expectedLogResult.logFailure) ) {
		    printf2("Verify record\n");
		    if (rc > 0) {
			pass_testcases++;
			printf2("AUDIT PASS ");
		    } else {
			fail_testcases++;
			debug_expected(&ld);
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

		printf2prime(
		    ": '%s' [logSuccess=%x, logFailure=%x, successCase=%x]\n", 
		    ld.testName,
		    filterconf_tests[i].expectedLogResult.logSuccess,
		    filterconf_tests[i].expectedLogResult.logFailure, 
		    ld.successCase);
	}

	printf2("PASSED = %i, FAILED = %i\n", pass_testcases, fail_testcases);
	goto EXIT;


EXIT_ERROR:
	printf1("ERROR: Test aborted\n");

EXIT:
	term_audit_conf();

	if (l_user)
		free(l_user);
	if (u_user)
		free(u_user);
	if (testcase)
		free(testcase);

	return 0;
}
