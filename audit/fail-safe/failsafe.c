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
**  FILE   : failsafe.c
**
**  PURPOSE: This file contains the main body of the Linux Auditing
**           System test suite for the failsafe test.
**           This file, as well as the accompanying headers in the
**           ./includes/ directory and the helper utility functions in
**           the ./utils/ directory, form a framework that facilitates a
**           robustly modular test suite.
**
**           Each test is written as an independent function to be
**           called by the main body in this file.  This design
**           conveniently allows multiple developers to implement
**           test functions for separate failsafe tests (located in the
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

#include "failsafe.h"
#include "failsafe_array.h"

#include <pwd.h>
#include <getopt.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/msg.h>


int pass_testcases = 0;
int fail_testcases = 0;
int skip_testcases = 0;

int debug = 2;

uid_t login_uid = 0;

char cwd[PATH_MAX];

char test_exe[PATH_MAX];

char notify_exe[PATH_MAX];

int semid = -1;

int msgid = -1;
failsafe_msgbuf msg;
struct msgbuf *msgbufp = (struct msgbuf *)&msg;



// failsafe usage
void failsafe_usage() {

	printf(
		"usage:\n"
		"failsafe   [--user | -u TEST-USER]\n"
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


// failsafe_main
int failsafe_main(int argc, char **argv) {

#ifdef __IX86
	int arch = AUDIT_ARCH_I386;
#endif
#ifdef __PPC32
	int arch = AUDIT_ARCH_PPC;
#endif
#ifdef __PPC64
	int arch = AUDIT_ARCH_PPC64;
#endif
#ifdef __S390
	int arch = AUDIT_ARCH_S390;
#endif
#ifdef __S390X
        int arch = AUDIT_ARCH_S390X;
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

	char *notify = NULL;

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
				failsafe_usage();
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
				failsafe_usage();
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
		for (i = 0; i < (sizeof(failsafe_tests) / sizeof(failsafe_data)); i++) {
			if ((rc = strcmp(testcase, failsafe_tests[i].testName)) == 0 )
				break;
		}
		if (rc) {
			printf1("ERROR: failsafe test '%s' does not exit\n", testcase);
			goto EXIT_ERROR;
		}
	}

	// Save the executable name/path for tests
	if (realpath(argv[0], test_exe) == 0) {
		printf1("ERROR: realpath failed - errno=%d %s\n", errno, strerror(errno));
	}

	// Create the executable name for the notify program
	strcpy(notify_exe, test_exe);
	*(notify = strrchr(notify_exe, '/') + 1) = '\0';
	strcat(notify_exe, NOTIFY_PGM);

	// Create the semaphore - access: rw-rw-rw-
	//                                110110110 = 1B6x = 438
	if ((semid = semget(IPC_PRIVATE, 1, 438)) == -1) {
		printf1("ERROR: semget failed - errno=%d %s\n", errno, strerror(errno));
		goto EXIT_ERROR;
	}
	printf5("Semaphore created - id=%d\n", semid);

	// Create the message queue - access: rw-rw-rw-
	//                                    110110110 = 1B6x = 438
	if ((msgid = msgget(IPC_PRIVATE, 438)) == -1) {
		printf1("ERROR: msgget failed - errno=%d %s\n", errno, strerror(errno));
		goto EXIT_ERROR;
	}
	printf5("Message queue created - id=%d\n", msgid);

	// Loop through failsafe tests
	for (i = 0; i < (sizeof(failsafe_tests) / sizeof(failsafe_data)); i++) {
		// Run exactly one test case?
		if (testcase && (strcmp(testcase, failsafe_tests[i].testName) != 0 ))
			continue;

		memset(&ld, 0, sizeof(laus_data));
		ld.successCase = TRUE;
		ld.msg_arch    = arch;
		ld.testName    = failsafe_tests[i].testName;

		printf2("Performing test '%s'\n", failsafe_tests[i].testName);
		rc = failsafe_tests[i].testPtr(&ld, u_passwd->pw_uid, u_passwd->pw_gid);
		if (rc != 0) {
			printf1("ERROR: Test '%s' failed\n", failsafe_tests[i].testName);
			goto EXIT_ERROR;
		}
	}

	printf2("PASSED = %i, FAILED = %i\n", pass_testcases, fail_testcases);
	goto EXIT;


EXIT_ERROR:
	printf1("ERROR: Test aborted\n");

EXIT:
	if (l_user)
		free(l_user);
	if (u_user)
		free(u_user);
	if (testcase)
		free(testcase);

	return rc;
}


// failsafe_notify usage
void failsafe_notify_usage() {

	NOTIFY_PGM_PRINT(1,
		"usage:\n"
		"failsafe_notify    --msgid | -m MESSAGE-ID\n"
		"                  [--semid | -s SEMAPHORE-ID]\n"
		"                  [--rc    | -r RETURN-CODE]\n"
		"                  [--debug | -d DEBUG-LEVEL]\n"
		"                  [--help | -h]\n"
		"  MESSAGE-ID:    The id of the message queue to use.\n"
		"  SEMAPHORE-ID:  The id of the semaphore to use.\n"
		"  RETURN-CODE:   The return code that the program should\n"
		"                 terminate with. Default: 0\n"
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


// failsafe_notify_main
int failsafe_notify_main(int argc, char **argv) {

	int rc = 0;

	int option;
	char short_opts[] = "d:hm:r:s:";
	struct option long_opts[] = {
			{"debug", 1, 0, 'd'},
			{"help",  0, 0, 'h'},
			{"msgid", 1, 0, 'm'},
			{"rc",    1, 0, 'r'},
			{"semid", 1, 0, 's'},
			{0, 0, 0, 0},
		};

	int notify_rc = 0;

	struct sembuf semops;


	// Iterate through command line options
	while ((option = getopt_long(argc, argv, short_opts, long_opts, NULL)) != -1) {
		switch (option) {
			case 'd':
				// Debug level specified
				debug = atoi(optarg);
				// BUGBUG: This if statement needs to be modified to handle
				// non-numeric input correctly.
				if ((debug < 0) || (debug > 9)) {
					NOTIFY_PGM_PRINT(1, "ERROR: Debug level '%s' is not valid\n", optarg);
					goto EXIT_ERROR;
				}

				break;

			case 'h':
				// Help requested
				failsafe_notify_usage();
				goto EXIT;

				break;

			case 'm':
				// Message id specified
				msgid = atoi(optarg);

				break;

			case 'r':
				notify_rc = atoi(optarg);

				break;

			case 's':
				// Semaphore id specified
				semid = atoi(optarg);

				break;

			default:
				failsafe_notify_usage();
				goto EXIT_ERROR;

				break;
		}
	}

	// Be sure we received the message id
	if (msgid == -1) {
		NOTIFY_PGM_PRINT(1, "ERROR: You must supply a message id\n");
		goto EXIT_ERROR;
	}
	NOTIFY_PGM_PRINT(5, "Message queue to use - id=%d\n", msgid);

	// Check if we received a semaphore id
	if (semid != -1) {
		NOTIFY_PGM_PRINT(5, "Semaphore to use - id=%d\n", semid);
	}

	// Be sure the test was invoked as root
	if (getuid() != 0) {
		NOTIFY_PGM_PRINT(1, "ERROR: You must be running as 'root' to run this command\n");
		goto EXIT_ERROR;
	}

	if (semid != -1) {
		// Decrement the semaphore value to notify the test
		semops.sem_num = 0;
		semops.sem_op  = -1;
		semops.sem_flg = IPC_NOWAIT;
		if ((rc = semop(semid, &semops, 1)) == -1) {
			NOTIFY_PGM_PRINT(1, "ERROR: semop failed - errno=%d %s\n", errno, strerror(errno));
			goto EXIT_ERROR;
		}

		NOTIFY_PGM_PRINT(5, "Semaphore successfully decremented\n");
	}

	goto EXIT;

EXIT_ERROR:
	NOTIFY_PGM_PRINT(1, "ERROR: Notify aborted\n");

EXIT:
	NOTIFY_PGM_PRINT(5, "Notify program exiting - rc=%d\n", notify_rc);

	return notify_rc;
}


// main
int main(int argc, char **argv) {

	int rc = 0;

	char *name = NULL;


	// Get the program name as invoked
	name = strrchr(argv[0], '/');
	if (!name)
		name = argv[0];
	else
		name++;
	
	if (strcmp(name, TEST_PGM) == 0) {
		// Test program invoked
		rc = failsafe_main(argc, argv);
	}
	else if (strcmp(name, NOTIFY_PGM) == 0) {
		// Notify program invoked
		rc = failsafe_notify_main(argc, argv);
	}

	return rc;
}
