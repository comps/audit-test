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
**  FILE   : libpam.h 
**
**  PURPOSE: This file contains the function declarations for each of
**           the test functions as defined in their own source code
**           files in ../tests.
**
**
**  HISTORY:
**    08/03 originated by Dustin Kirkland (k1rkland@us.ibm.com)
**
**********************************************************************/

#ifndef _PAMPROGRAMS_H
#define _PAMPROGRAMS_H

typedef struct {
  char* command;
  char* command_args;
  char* audit_text;
  char* cleanup;
  char* cleanup_args;
} pamprogram_commands;

typedef struct {
    int		(*testPtr)(audit_data *);
    char	*testName;
    audit_data	*dataPtr;
} pam_data;

int test_login(audit_data* dataPtr);
int test_sshd(audit_data* dataPtr);
int test_su(audit_data* dataPtr);
int test_vsftpd(audit_data* dataPtr);

void test_pamprogram(pamprogram_commands command_data[],
			 int array_size,
			 audit_data* dataPtr);

int prePAMProgram(audit_data* dataPtr);
int postPAMProgram(audit_data* dataPtr);
int runPAMProgram( audit_data* dataPtr, char* command );
int verifyPAMProgram( audit_data* dataPtr );

#endif

