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
**  FILE       : test_rollover.c
**
**  PURPOSE    : Used to test the audit failsafe support.
**
**  DESCRIPTION: This file contains various routines used to test
**  the failsafe support.  The functions build into the laus_test
**  framework to provide a basis to verify that the Linux Audit System
**  performs as designed.
**
**  Each function will documents its execution.
**
**  HISTORY    :
**    09/03 Originated by Tom Lendacky (toml@us.ibm.com)
**
**********************************************************************/

#include "includes.h"

#include "failsafe.h"
#include "failsafe_config.h"
#include "failsafe_utils.h"

#include <sys/ipc.h>
#include <sys/sem.h>

#include <signal.h>
#include <sys/wait.h>

#include <laus.h>

#include <time.h>

extern int semid;
extern int msgid;


/**********************************************************************
**  Function: test_rollover_handler
**    Provides support for catching a signal indicating an error during
**    the testcase.
**
**   1) Just catches a signal.
**
**********************************************************************/
void test_rollover_handler(int  i) {

	printf5("Signal handler in control - signal=%d\n", i);
}


/**********************************************************************
**  Function: test_rollover
**    Tests the functionality of the log file rollover support.  This
**    includes the notification function, symlink update and actual
**    rollover to the new file.
**
**   1) Performs pre-processing, including establishing a signal
**      handler, incrementing the semaphore and establishing the audit
**      configuration (with small pre-allocated binary files in order
**      to easily test the rollover function).
**   2) Forks a process that writes repeated user-message audit records
**      to cause the log files to rollover.
**   3) Verifies the rollover function by waiting for a successful
**      decrement of the semaphore, insuring the symlink points to the
**      new log file and verifying the new log file contains a
**      user-message audit record.
**   4) Performs post-processing, including resetting the audit
**      configuration and displaying all output messages from the
**      notification program.
**
**********************************************************************/
int test_rollover(laus_data *ld, uid_t uid, gid_t gid) {

	int rc = 0;

	pid_t parent_pid = 0;

	pid_t pid = 0;

	char old_log[PATH_MAX];
	char new_log[PATH_MAX];

	struct sembuf semops;

	failsafe_msgbuf msg;
	struct msgbuf *msgbufp = (struct msgbuf *)&msg;


	// Establish the signal handler
	if (signal(SIGHUP, test_rollover_handler) == SIG_ERR) {
		printf1("ERROR: signal failed\n");
		rc = -1;
		goto EXIT;
	}

	// Increment the semaphore value in preparation for the test
	semops.sem_num = 0;
	semops.sem_op  = 1;
	semops.sem_flg = 0;
	if ((rc = semop(semid, &semops, 1)) == -1) {
		printf("ERROR: semop failed - errno=%d %s\n", errno, strerror(errno));
		goto EXIT;
	}

	// Initialize the audit configuration
	//   Notify program rc=0
	//   Notify program to decrement semaphore
	if ((rc = init_audit_conf(0, semid)) < 0) {
		printf1("ERROR: Unable to initialize the audit configuration\n");
		goto EXIT;
	}

	// Do pre-failsafe call work
	printf5("Calling pre_failsafe_call\n");
	if ((rc = pre_failsafe_call(ld)) != 0) {
		printf1("ERROR: pre_failsafe_call failed - rc=%d\n", rc);
		goto EXIT;
	}

	// Get the current log file name
	if (realpath(CURRENT_AUDIT_LOG, old_log) == 0) {
		printf1("ERROR: realpath failed - errno=%d %s\n", errno, strerror(errno));
		rc = -1;
		goto EXIT;
	}


        //Set beginning time for expected record
        ld->begin_r_time = time( NULL )-2; // MH: Start two seconds
                                          // before current tim

	// Fork the process
	printf5("Preparing to fork child process\n");
	parent_pid = getpid();
	pid = fork();
	if (pid < 0) {
		printf1("ERROR: fork failed - errno=%d %s\n", errno, strerror(errno));
		rc = -1;
		goto EXIT;
	}
	else if (pid == 0) {
		// Child process
		if ((rc = laus_open(NULL)) == 0) {
			int i;
			// The bin log size should be small enough that a few user-msg
			// entries should cause a log file switch
			printf5("Performing %d user-msg entries to generate log file rollover\n", AUDIT_SWITCH_COUNT);
			for (i = 0; i < AUDIT_SWITCH_COUNT; i++) {
				if ((rc = laus_log(NO_TAG, AUDIT_USER_MSG, i+1)) != 0)
					break;
			}

			if (rc == 0) {
				printf5("User-msg entries generated, sleeping...\n");
				sleep(20);
			}
			else {
				printf1("ERROR: laus_log failed - errno=%d %s\n", errno, laus_strerror(errno));
			}
		}
		else {
			printf1("ERROR: laus_open failed - errno=%d %s\n", errno, laus_strerror(errno));
		}

		// If we make it to here either there was an error or the notify
		//   hasn't occurred (our parent hasn't killed us) before we woke
		//   up. Send a SIGHUP to the parent to terminate the semaphore wait
		//   (but sleep for a bit to be sure to give the parent time to issue the semop).
		printf5("ERROR: Log file rollover did not occur\n");
		sleep(5);
		kill(parent_pid, SIGHUP);
		exit(0);
	}
	// Use new PID
	printf5("Fork successful - pid=%d\n", pid);
	ld->msg_pid = pid;

	printf5("Waiting on semaphore for indication of notify success\n");
	semops.sem_num = 0;
	semops.sem_op  = 0;
	semops.sem_flg = 0;
	if ((rc = semop(semid, &semops, 1)) == 0) {
		// Kill the child now
		kill(pid, SIGKILL);

		// Sleep for just a bit to insure the rollover function completes
		sleep(10);

                // Set end time for expected audit record
                ld->end_r_time = time( NULL )+2; // MH: 2 seconds past current
                                        // time 
		// Get the current log file name
		if (realpath(CURRENT_AUDIT_LOG, new_log) == 0) {
			printf1("ERROR: realpath failed - errno=%d %s\n", errno, strerror(errno));
			rc = -1;
			goto EXIT;
		}


		printf5("Old log file: %s\n", old_log);
		printf5("New log file: %s\n", new_log);
		if (strcmp(old_log, new_log) != 0) {
			log_options log_opt = { TRUE, TRUE };

			printf5("Log file rollover successful, checking log file contents\n");

			// Set the data
			ld->msg_type = AUDIT_MSG_TEXT;
			ld->laus_var_data.textData.data = mysprintf(AUDIT_USER_MSG, AUDIT_SWITCH_COUNT);
			// Verify the last message is present
			verifyLog(ld, log_opt);
		}
		else {
			printf5("Log file rollover not successful\n");
			fail_testcases++;
			printf2("AUDIT FAIL: '%s'\n", ld->testName);
		}
	}
	else {
		if (errno == EINTR) {
			printf5("Log file rollover not successful\n");
			fail_testcases++;
			printf2("AUDIT FAIL: '%s'\n", ld->testName);
			rc = 0;
		}
		else {
			printf1("ERROR: semop failed - errno=%d %s\n", errno, strerror(errno));
			kill(pid, SIGKILL);
			goto EXIT;
		}
	}

	// Do post-failsafe call work
	printf5("Calling post_failsafe_call\n");
	if ((rc = post_failsafe_call(ld)) != 0) {
		printf1("ERROR: post_failsafe_call failed - rc=%d\n", rc);
		goto EXIT;
	}

EXIT:
	// Do cleanup work here
	// Read all notify program messages
	printf5("Receiving any and all notify program messages\n");
	memset(&msg, 0, sizeof(msg));
	while (msgrcv(msgid, msgbufp, sizeof(msg.mtext), 0, IPC_NOWAIT) > 0) {
		printf5("Notify program message: ");
		printf_level(msg.mtype, msg.mtext.msg);

		memset(&msg, 0, sizeof(msg));
	}

	// Restore audit configuration
	term_audit_conf();

	printf5("Returning from test - rc=%d\n", rc);

	return rc;
}
