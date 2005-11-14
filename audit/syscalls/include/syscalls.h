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
**  FILE   : syscalls.h 
**
**  PURPOSE: This file contains the function declarations for each of
**           the test functions as defined in their own source code
**           files in ../tests
**
**
**  HISTORY:
**    05/03 originated by Dan Jones (danjones@us.ibm.com)
**    06/03 furthered by Dustin Kirkland (k1rkland@us.ibm.com
**
**********************************************************************/

#ifndef _SYSCALLS_H
#define _SYSCALLS_H

#include "context.h"

/*
 * Default Values
 */
#define DEFAULT_TEST_USER "nobody"
#define XATTR_TEST_VALUE "text/plain"

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

/*
 * Limits
 */
#define TESTCASE_NAME_MAX 25
#define MAX_ARG_SIZE 30                 /* test_execve */
#define MAX_WAIT_TIME_FOR_CHILD 60      /* test_execve */

/* 
 * Global variables
 */
extern int helper_uid; /* user for testing EPERM, EACCES errors */

/*
 * Syscall Test Function Prototypes
 *
 * When adding a new test, you must also update the test lookup table
 * in syscalls_table.c.
 */
int test_access(struct audit_data *);
int test_adjtimex(struct audit_data *);
int test_bind(struct audit_data *);
int test_capset(struct audit_data *);
int test_chdir(struct audit_data *);
int test_chmod(struct audit_data *);
int test_chown(struct audit_data *);
int test_chown16(struct audit_data *);
int test_chown32(struct audit_data *);
int test_chroot(struct audit_data *);
int test_clone(struct audit_data *);
int test_clone2(struct audit_data *);
int test_creat(struct audit_data *);
int test_delete_module(struct audit_data *);
int test_execve(struct audit_data *);
int test_fchdir(struct audit_data *);
int test_fchmod(struct audit_data *);
int test_fchown(struct audit_data *);
int test_fchown16(struct audit_data *);
int test_fchown32(struct audit_data *);
int test_fork(struct audit_data *);
int test_fremovexattr(struct audit_data *);
int test_fsetxattr(struct audit_data *);
int test_ftruncate(struct audit_data *);
int test_ftruncate64(struct audit_data *);
int test_init_module(struct audit_data *);
int test_ioctl(struct audit_data *);
int test_ioperm(struct audit_data *);
int test_iopl(struct audit_data *);
int test_kill(struct audit_data *);
int test_lchown(struct audit_data *);
int test_lchown16(struct audit_data *);
int test_lchown32(struct audit_data *);
int test_link(struct audit_data *);
int test_lremovexattr(struct audit_data *);
int test_lsetxattr(struct audit_data *);
int test_mkdir(struct audit_data *);
int test_mknod(struct audit_data *);
int test_mount(struct audit_data *);
int test_msgctl(struct audit_data *);
int test_msgget(struct audit_data *);
int test_msgrcv(struct audit_data *);
int test_msgsnd(struct audit_data *);
int test_open(struct audit_data *);
int test_ptrace(struct audit_data *);
int test_reboot(struct audit_data *);
int test_removexattr(struct audit_data *);
int test_rename(struct audit_data *);
int test_rmdir(struct audit_data *);
int test_semctl(struct audit_data *);
int test_semget(struct audit_data *);
int test_semop(struct audit_data *);
int test_semtimedop(struct audit_data *);
int test_setdomainname(struct audit_data *);
int test_setfsgid(struct audit_data *);
int test_setfsgid16(struct audit_data *);
int test_setfsgid32(struct audit_data *);
int test_setfsuid(struct audit_data *);
int test_setfsuid16(struct audit_data *);
int test_setfsuid32(struct audit_data *);
int test_setgid(struct audit_data *);
int test_setgid16(struct audit_data *);
int test_setgid32(struct audit_data *);
int test_setgroups(struct audit_data *);
int test_setgroups16(struct audit_data *);
int test_setgroups32(struct audit_data *);
int test_sethostname(struct audit_data *);
int test_setpgid(struct audit_data *);
int test_setpriority(struct audit_data *);
int test_setregid(struct audit_data *);
int test_setregid16(struct audit_data *);
int test_setregid32(struct audit_data *);
int test_setresgid(struct audit_data *);
int test_setresgid16(struct audit_data *);
int test_setresgid32(struct audit_data *);
int test_setresuid(struct audit_data *);
int test_setresuid16(struct audit_data *);
int test_setresuid32(struct audit_data *);
int test_setreuid(struct audit_data *);
int test_setreuid16(struct audit_data *);
int test_setreuid32(struct audit_data *);
int test_setrlimit(struct audit_data *);
int test_setsid(struct audit_data *);
int test_settimeofday(struct audit_data *);
int test_setuid(struct audit_data *);
int test_setuid16(struct audit_data *);
int test_setuid32(struct audit_data *);
int test_setxattr(struct audit_data *);
int test_shmat(struct audit_data *);
int test_shmctl(struct audit_data *);
int test_shmdt(struct audit_data *);
int test_shmget(struct audit_data *);
int test_swapoff(struct audit_data *);
int test_swapon(struct audit_data *);
int test_symlink(struct audit_data *);
int test_syslog(struct audit_data *);
int test_tkill(struct audit_data *);
int test_truncate(struct audit_data *);
int test_truncate64(struct audit_data *);
int test_umask(struct audit_data *);
int test_unlink(struct audit_data *);
int test_utime(struct audit_data *);
int test_utimes(struct audit_data *);
int test_vfork(struct audit_data *);

int lookup_testcase(int (**test_h)(), char *testcase);

#endif
