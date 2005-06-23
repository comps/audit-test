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
#include "../../include/includes.h"

typedef struct {
  char* command;
  char* command_args;
  char* audit_text;
  char* cleanup;
  char* cleanup_args;
} trustedprogram_commands;

int test_at(laus_data* dataPtr);
int test_atd(laus_data* dataPtr);
int test_cron(laus_data* dataPtr);
int test_crontab(laus_data* dataPtr);
int test_gpasswd(laus_data* dataPtr);
int test_groupadd(laus_data* dataPtr);
int test_groupdel(laus_data* dataPtr);
int test_groupmod(laus_data* dataPtr);
int test_passwd(laus_data* dataPtr);
int test_useradd(laus_data* dataPtr);
int test_userdel(laus_data* dataPtr);
int test_usermod(laus_data* dataPtr);


void test_trustedprogram(trustedprogram_commands command_data[],
			 int array_size,
			 laus_data* dataPtr);

int runTrustedProgramAndVerify(laus_data* dataPtr, char* command);
int runTrustedProgramWithoutVerify( laus_data* dataPtr, char* command );
int verifyTrustedProgram( laus_data* dataPtr );

int SystemX(char* command);
#define system SystemX

#endif

