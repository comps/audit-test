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
**  FILE   : global.h
**
**  PURPOSE: This file contains the variable definitions that are 
**           globally available within the Linux Auditing System
**           test suite.
**
**
**  HISTORY:
**    05/03 originated by Dan Jones (danjones@us.ibm.com)
**    06/03 furthered by Dustin Kirkland (k1rkland@us.ibm.com
**    07/03 furthered by Michael A. Halcrow <mike@halcrow.us>
**
**********************************************************************/

#ifndef _GLOBALS_H
#define _GLOBALS_H

#if defined(__IX86)
#define AUDIT_ARCH  AUDIT_ARCH_I386
#elif defined(__PPC32)
#define AUDIT_ARCH  AUDIT_ARCH_PPC
#elif defined(__PPC64)
#define AUDIT_ARCH  AUDIT_ARCH_PPC64
#elif defined(__S390X)
#define AUDIT_ARCH  AUDIT_ARCH_S390X
#elif defined(__S390)
#define AUDIT_ARCH  AUDIT_ARCH_S390
#elif defined(__X86_64)
#define AUDIT_ARCH  AUDIT_ARCH_X86_64
#elif defined(__IA64)
#define AUDIT_ARCH  AUDIT_ARCH_IA64
#else
#define AUDIT_ARCH  -1
#endif

#define NO_RETURN_CODE          -2
#define NO_PID_CHECK            -2
#define NO_ID_CHECK             -2
#define NO_FORK			-3 

//
// Global variables
//
extern int logOptionsIndex;
//
// If an error is detected with the syscall being tested, an ERROR
// message is output and the global test_rc variable is set. This
// allows the test to continue for the rest of the syscalls.
//
// If an error is detected with any of the test setup functions,
// the test case aborts immediately since the test environment is
// no longer valid.
//
extern int test_rc;
//
// Constant resource names
//
// Temp files are dynamically generated as /tmp/laustestxxxxxx.
// BUGBUG: tempdir should be read from environment variable
// 
// static char* tempdir  = "/tmp";
// static char* tempprefix = "laus";

extern int debug;
extern char cwd[PATH_MAX];

// 
// The login uid must be passed into the test since it
// it cannot be obtained via a system call. The login
// uid will be checked against the value stored in an
// audit record.
// 
extern uid_t login_uid;

//
// The security relevant tests attempt to test the EPERM and EACCES
// error conditions.  To do this, we create a globally available test
// user.
extern char* helper;
extern int helper_uid;
extern char* helper_homedir;

#endif
