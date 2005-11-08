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
**  FILE       : test_rollover_failure.c
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

extern int semid;
extern int msgid;

int signal_caught = 0;


/**********************************************************************
**  Function: test_rollover_failure_handler
**    Provides support for catching a signal indicating an error during
**    the testcase.
**
**   1) Just catches a signal and save the signal value.
**
**
**********************************************************************/
void test_rollover_failure_handler(int  i) {

	printf5("Signal handler in control - signal=%d\n", i);
	signal_caught = i;
}


/**********************************************************************
**  Function: test_rollover_failure
**    Tests the functionality of the log file rollover failure support.
**    This implies that auditing processes should be blocked when the
**    rollover support tries to return to a file that was not
**    successfully processed by the notify program (a non-zero return
**    code is returned from the notify program).
**
**   1) Performs pre-processing, including establishing a signal
**      handler, incrementing the semaphore, lowering the kernel audit
**      buffer (max-messages) and establishing the audit configuration
**      (with small pre-allocated binary files in order to easily test
**      the rollover function and telling the notify program to return
**      a non-zero return code).
**   2) Forks a process that writes repeated user-message audit records
**      to cause the log files to rollover.  But before it does this,
**      it sleeps for a bit to give the other processes time to setup.
**      If this process does not block because of unsuccessful rollover
**      failure support then this process will signal the parent with
**      a SIGHUP.
**   3) Forks a second process that detaches from auditing so as not
**      to be blocked because of the rollover failure support.  But
**      before it does this, it sleeps for a bit to give the parent
**      process time to setup.  After detaching from auditing this
**      process will sleep for a while to allow the first process
**      to do it's thing.  If the first process gets blocked because
**      of successful rollover failure support then this process will
**      eventually wake up and signal the parent with a SIGINT.
**   4) Detaches from auditing so as not to be blocked because of the 
**      rollover failure support.  Verifies the rollover failure support
**      by waiting for a successful decrement of the semaphore (which
**      will never happen - but since a semop call will return if the
**      process is signaled it allows the test to easily process the
**      signal which will indicate success or failure).
**         SIGQUIT - an error during test setup
**         SIGHUP  - rollover failure support failed
**         SIGINT  - rollover failure support succeeded
**   MH: TODO: audit_stop() now uses /etc/init.d/audit stop.  If there
**   are any unexplained test failures here, look into this as one
**   possible reason.
**   5) Performs post-processing, including resetting the kernel audit
**      buffer (max-messages), killing the audit daemon using 'kill -9'
**      because audit_stop() (which uses killall) won't work if rollover
**      failure support worked, resetting the audit configuration and
**      displaying all output messages from the notification program(s).
**
**********************************************************************/
int test_rollover_failure(laus_data *ld, uid_t uid, gid_t gid) {

	int rc = 0;

	pid_t parent_pid = 0;

	pid_t pid_1 = 0;
	pid_t pid_2 = 0;

	struct sembuf semops;

	failsafe_msgbuf msg;
	struct msgbuf *msgbufp = (struct msgbuf *)&msg;


	// Establish the signal handlers
	if (signal(SIGHUP, test_rollover_failure_handler) == SIG_ERR) {
		printf1("ERROR: signal failed\n");
		rc = -1;
		goto EXIT;
	}
	if (signal(SIGINT, test_rollover_failure_handler) == SIG_ERR) {
		printf1("ERROR: signal failed\n");
		rc = -1;
		goto EXIT;
	}
	if (signal(SIGQUIT, test_rollover_failure_handler) == SIG_ERR) {
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
	//   Notify program rc=-1
	//   Notify program to NOT decrement semaphore
	if ((rc = init_audit_conf(-1, -1)) < 0) {
		printf1("ERROR: Unable to initialize the audit configuration\n");
		goto EXIT;
	}

	// Do pre-failsafe call work
	printf5("Calling pre_failsafe_call\n");
	if ((rc = pre_failsafe_call(ld)) != 0) {
		printf1("ERROR: pre_failsafe_call failed - rc=%d\n", rc);
		goto EXIT;
	}

	// Fork the process
	printf5("Preparing to fork child process\n");
	parent_pid = getpid();
	pid_1 = fork();
	if (pid_1 < 0) {
		printf1("ERROR: fork failed - errno=%d %s\n", errno, strerror(errno));
		rc = -1;
		goto EXIT;
	}
	else if (pid_1 == 0) {
		// Child process
		printf5("Child process executing - pid=%d\n", getpid());
		
		// Give the other processes (parent and other child) time to do their thing
		sleep(10);
		
		// Lower the kernel max-messages value
		printf5("Lowering the max-message value (dev.audit.max-messages)\n");
		if ((rc = system("/sbin/sysctl -w dev.audit.max-messages=10 >/dev/null")) == -1) {
			printf1("ERROR: sysctl command failed\n");
			kill(parent_pid, SIGQUIT);
		}
	
		// Do some audit stuff...
		if ((rc = laus_open(NULL)) == 0) {
			int i;
			// The bin log size should be small enough that a few user-msg
			//   entries should cause a log file switch
			// The idea is to switch multiple times and have the notify program
			//   issue a bad return code each time causing the laus_log function
			//   call to eventually be suspended.
			printf5("Performing %d user-msg entries to generate log file rollover\n", AUDIT_SWITCH_COUNT * 10);
			for (i = 0; i < AUDIT_SWITCH_COUNT * 10; i++) {
				if ((rc = laus_log(NO_TAG, AUDIT_USER_MSG, i+1)) != 0)
					break;
			}
		}
		else {
			printf1("ERROR: laus_open failed - errno=%d %s\n", errno, laus_strerror(errno));
			kill(parent_pid, SIGQUIT);
		}

		// If we make it to here either there was an error or logging wasn't 
		//   suspended. Send a SIGHUP to the parent to terminate the semaphore wait
		//   (but sleep for a bit to be sure to give the parent time to issue the semop).
		printf5("ERROR: Log file suspension did not occur\n");
		kill(parent_pid, SIGHUP);
		exit(0);
	}
	printf5("Fork successful - pid=%d\n", pid_1);

	// Fork the process
	printf5("Preparing to fork child process\n");
	pid_2 = fork();
	if (pid_2 < 0) {
		printf1("ERROR: fork failed - errno=%d %s\n", errno, strerror(errno));
		rc = -1;
		goto EXIT;
	}
	else if (pid_2 == 0) {
		// Child process
		printf5("Child process executing - pid=%d\n", getpid());

		// Give the parent time to do it's thing
		sleep(5);
		
		// Detach from auditing to "free" up this process
		printf5("Child process detaching from auditing - pid=%d\n", getpid());
		if ((laus_open(NULL) != 0) || (laus_detach() != 0)) {
			printf1("ERROR: laus error - errno=%d %s\n", errno, laus_strerror(errno));
			kill(parent_pid, SIGQUIT);
		}

		// Give the first child time to hose the audit system
		sleep(30);

		// If we make it to here then the other child was suspended and we have
		//   a success. Send a SIGINT to the parent to terminate the semaphore wait.
		printf5("Log file suspension occurred\n");
		kill(parent_pid, SIGINT);
		exit(0);
	}
	printf5("Fork successful - pid=%d\n", pid_2);

	// Detach from auditing to "free" up this process
	printf5("Parent process detaching from auditing\n");
	if ((laus_open(NULL) != 0) || (laus_detach() != 0)) {
		printf1("ERROR: laus error - errno=%d %s\n", errno, laus_strerror(errno));
		kill(pid_1, SIGKILL);
		kill(pid_2, SIGKILL);
		goto EXIT;
	}

	// Wait on the semaphore - The notify program will not decrement
	//   the semaphore, one of the children's kill statements will
	//   cause it to return.
	printf5("Wait on semaphore for indication of notify success\n");
	printf1("***  This can take a while, be patient  ***\n")
	semops.sem_num = 0;
	semops.sem_op  = 0;
	semops.sem_flg = 0;
	if ((rc = semop(semid, &semops, 1)) == 0) {
		// This shouldn't happen
		printf1("ERROR: semop succeeded somehow\n");
		kill(pid_1, SIGKILL);
		kill(pid_2, SIGKILL);
		rc = -1;
		goto EXIT;
	}
	else {
		if (errno == EINTR) {
			// Kill the children now
			kill(pid_1, SIGKILL);
			kill(pid_2, SIGKILL);

			rc = 0;

			if (signal_caught == SIGHUP) {
				// SIGHUP - first child signal means a failure
				printf5("Log file rollover failure support not successful\n");
				fail_testcases++;
				printf2("AUDIT FAIL: '%s'\n", ld->testName);
			}
			else if (signal_caught == SIGINT) {
				// SIGINT - second child signal means a success
				printf5("Log file rollover failure support successful\n");
				pass_testcases++;
				printf2("AUDIT PASS: '%s'\n", ld->testName);
			}
			else {
				// Testcase error
				printf1("ERROR: Testcase error signalled\n");
				rc = -1;
				goto EXIT;
			}
		}
		else {
			printf1("ERROR: semop failed - errno=%d %s\n", errno, strerror(errno));
			goto EXIT;
		}
	}

	// Reset the kernel max-messages value
	printf5("Resetting the max-message value (dev.audit.max-messages)\n");
	if ((rc = system("/sbin/sysctl -w dev.audit.max-messages=1024 >/dev/null")) == -1) {
		printf1("ERROR: sysctl command failed\n");
		goto EXIT;
	}

	// A killall command will hang now, kill the audit daemon with -9
	printf5("Using 'kill -9' to kill the audit daemon\n");
#ifdef __PPC64
        system("/bin/kill -9 `/bin/pidof auditd64` >/dev/null 2>&1");
#else
	system("/bin/kill -9 `/bin/pidof auditd` >/dev/null 2>&1");
#endif
	
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
