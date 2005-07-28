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

#define DEFAULT_TEST_USER "nobody"
#define TESTCASE_NAME_MAX 25

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

// For syscalls/tests/test_execve.c
#define MAX_ARG_SIZE 30
#define MAX_WAIT_TIME_FOR_CHILD 60

#ifndef __PPC
#define __NR_chown16 182
#define __NR_fchown16 95
#define __NR_lchown16 16
#define __NR_setfsgid16 139
#define __NR_setfsuid16 138
#define __NR_setgid16 46
#define __NR_setgroups16 81
#define __NR_setregid16 71
#define __NR_setresgid16 170
#define __NR_setresuid16 164
#define __NR_setreuid16 70
#define __NR_setuid16 23
#endif

#define SKIP_TEST_CASE          256
#define NO_RETURN_CODE          -2
#define NO_PID_CHECK            -2
#define NO_ID_CHECK             -2
#define NO_FORK			-3 

#ifndef HOST_NAME_MAX
#define HOST_NAME_MAX 255
#endif

#define XATTR_TEST_VALUE "text/plain"

//
// Global variables
//
extern int pass_testcases;
extern int fail_testcases;
extern int skip_testcases;
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

/*
 * Audit utility functions ops structure
 */
struct audit_utils audit_ops;

// Here is where we redefine the C language to act like Perl!
#define RUNCOMMANDORDIE( cmd, ... ) { char* command = mysprintf( cmd, ## __VA_ARGS__ ); if( ( rc = system( command ) ) == -1 ) { printf1( "Error running command: [%s]\n", command ); free( command ); goto EXIT_CLEANUP; } free( command ); }
#define RUNCOMMAND( cmd, ... ) { char* command = mysprintf( cmd, ## __VA_ARGS__ ); if( ( rc = system( command ) ) == -1 ) { printf1( "Error running command: [%s]\n", command ); } free( command ); }
#define printf1( msg, ... ) if (debug >= 1) { printf( "ENVIRONMENT " ); printf( msg, ## __VA_ARGS__ ); }
#define printf2( msg, ... ) if (debug >= 2) { printf("TEST " ); printf( msg, ## __VA_ARGS__ ); }
#define printf2prime( msg, ... ) if (debug >= 2) { printf( msg, ## __VA_ARGS__ ); }
#define printf3( msg, ... ) if (debug >= 3) { printf("WARNING " ); printf( msg, ## __VA_ARGS__ ); }
#define printf4( msg, ... ) if (debug >= 4) { printf("INFO " ); printf( msg, ## __VA_ARGS__ ); }
#define printf5( msg, ... ) if (debug >= 5) { printf("DEBUG " ); printf( msg, ## __VA_ARGS__ ); }
#define printf5prime( msg, ... ) if (debug >= 5) { printf( msg, ## __VA_ARGS__ ); }
#define printf8( msg, ... ) if (debug >= 8) { printf("AUDIT " ); printf( msg, ## __VA_ARGS__ ); }
#define printf8prime( msg, ... ) if (debug >= 8) { printf( msg, ## __VA_ARGS__ ); }
#define printf9( msg, ... ) if (debug >= 9) { printf("AUDIT DEBUG " ); printf( msg, ## __VA_ARGS__ ); }
#define printf9prime( msg, ... ) if (debug >= 9) { printf( msg, ## __VA_ARGS__ ); }
#define printf_level(lev,  msg, ... ) { switch (lev) { \
case 1: \
printf1( msg, ## __VA_ARGS__ ); \
break; \
case 2: \
printf2( msg, ## __VA_ARGS__ ); \
break; \
case 3: \
printf3( msg, ## __VA_ARGS__ ); \
break; \
case 4: \
printf4( msg, ## __VA_ARGS__ ); \
break; \
case 5: \
printf5( msg, ## __VA_ARGS__ ); \
break; \
case 8: \
printf8( msg, ## __VA_ARGS__ ); \
break; \
case 9: \
printf9( msg, ## __VA_ARGS__ ); \
break; \
} }
#define printf_level_prime(lev,  msg, ... ) { switch (lev) { \
case 2: \
printf2prime( msg, ## __VA_ARGS__ ); \
break; \
case 5: \
printf5prime( msg, ## __VA_ARGS__ ); \
break; \
case 8: \
printf8prime( msg, ## __VA_ARGS__ ); \
break; \
case 9: \
printf9prime( msg, ## __VA_ARGS__ ); \
break; \
} }

#endif
