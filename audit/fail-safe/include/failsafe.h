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
**  FILE   : failsafe.h
**
**  PURPOSE: This file contains the function declarations for each of
**           the test functions as defined in their own source code
**           files in ../tests
**
**
**  HISTORY:
**    09/03 originated by Tom Lendacky (toml@us.ibm.com)
**
**********************************************************************/

#ifndef _FAILSAFE_H
#define _FAILSAFE_H

#include <sys/msg.h>


#define TEST_PGM		"failsafe"
#define NOTIFY_PGM		"failsafe_notify"
#define NOTIFY_PGM_PRINT(level, ...)						\
	do {									\
		printf_level(level, ## __VA_ARGS__);				\
		if (msgid != -1) {						\
			memset(&msg, 0, sizeof(msg));				\
			msg.mtype = level;					\
			sprintf(msg.mtext.msg, ## __VA_ARGS__);			\
			msgsnd(msgid, msgbufp, sizeof(msg.mtext), IPC_NOWAIT);	\
		}								\
	} while (0);

#define AUDIT_CONF		"/etc/audit/audit.conf"
#define FILTER_CONF		"/etc/audit/filter.conf"

#define CURRENT_AUDIT_LOG	"/var/log/audit"

#define AUDIT_ARCHIVE_CMD	"/usr/sbin/audbin -S %s -C"
#define AUDIT_FILE_SIZE		2048
#define AUDIT_USER_MSG		"FAILSAFE Test: Audit user-msg issued by test, count=%d"
#define AUDIT_SWITCH_COUNT	15


typedef struct {
	long mtype;
	struct {
		char msg[1024];
		char mnull[1];
	} mtext;
} failsafe_msgbuf;

// Function pointer definition for all failsafe tests
typedef int (*failsafeFunctions)(laus_data *ld, uid_t, gid_t);

// Failsafe test case data structure
typedef struct {
	failsafeFunctions	testPtr;
	char *			testName;
	laus_data *		dataPtr;
} failsafe_data;

int test_rollover(laus_data *ld, uid_t uid, gid_t gid);
int test_rollover_failure(laus_data *ld, uid_t uid, gid_t gid);

#endif
