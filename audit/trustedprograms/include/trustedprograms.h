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
**  FILE   : trustedprograms.h 
**
**  PURPOSE: This file contains the function declarations for each of
**           the test functions as defined in their own source code
**           files in ../tests.
**
**
**  HISTORY:
**    07/03 originated by Dan Jones (danjones@us.ibm.com)
**
**********************************************************************/

#ifndef _TRUSTEDPROGRAMS_H
#define _TRUSTEDPROGRAMS_H

int pass_testcases;
int fail_testcases;

typedef struct {
  char* command;
  char* command_args;
  char* audit_text;
  char* cleanup;
  char* cleanup_args;
} trustedprogram_commands;

typedef struct {
    int			(*testPtr)(struct audit_data *);
    char		*testName;
    struct audit_data	*dataPtr;
} trustedprogram_data;

int test_at(struct audit_data* dataPtr);
int test_atd(struct audit_data* dataPtr);
int test_cron(struct audit_data* dataPtr);
int test_crontab(struct audit_data* dataPtr);
int test_gpasswd(struct audit_data* dataPtr);
int test_groupadd(struct audit_data* dataPtr);
int test_groupdel(struct audit_data* dataPtr);
int test_groupmod(struct audit_data* dataPtr);
int test_passwd(struct audit_data* dataPtr);
int test_useradd(struct audit_data* dataPtr);
int test_userdel(struct audit_data* dataPtr);
int test_usermod(struct audit_data* dataPtr);


void test_trustedprogram(trustedprogram_commands command_data[],
			 int array_size,
			 struct audit_data* dataPtr);

int preTrustedProgram(struct audit_data* dataPtr);
int postTrustedProgram(struct audit_data* dataPtr);
int runTrustedProgramAndVerify(struct audit_data* dataPtr, char* command);
int runTrustedProgramWithoutVerify(struct audit_data* dataPtr, char* command );
int verifyTrustedProgram(struct audit_data* dataPtr );
int audit_grep_log(struct audit_data* dataPtr );

int ShadowTestSetup(int backupBool);
int SystemX(char* command);
#define system SystemX

#endif

