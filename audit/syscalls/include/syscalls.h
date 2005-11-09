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
extern char cwd[PATH_MAX];
extern int helper_uid; /* user for testing EPERM, EACCES errors */

/*
 * Syscall Test Function Prototypes
 *
 * When adding a new test, you must also update the test matrix in
 * syscalls_array.h.
 */
int test_access(laus_data* dataPtr);
int test_adjtimex(laus_data* dataPtr);
int test_bind(laus_data* dataPtr);
int test_capset(laus_data* dataPtr);
int test_chdir(laus_data* dataPtr);
int test_chmod(laus_data* dataPtr);
int test_chown(laus_data* dataPtr);
int test_chown16(laus_data* dataPtr);
int test_chown32(laus_data* dataPtr);
int test_chroot(laus_data* dataPtr);
int test_clone(laus_data* dataPtr);
int test_clone2(laus_data* dataPtr);
int test_creat(laus_data* dataPtr);
int test_delete_module(laus_data* dataPtr);
int test_execve(laus_data* dataPtr);
int test_fchdir(laus_data* dataPtr);
int test_fchmod(laus_data* dataPtr);
int test_fchown(laus_data* dataPtr);
int test_fchown16(laus_data* dataPtr);
int test_fchown32(laus_data* dataPtr);
int test_fork(laus_data* dataPtr);
int test_fremovexattr(laus_data* dataPtr);
int test_fsetxattr(laus_data* dataPtr);
int test_ftruncate(laus_data* dataPtr);
int test_ftruncate64(laus_data* dataPtr);
int test_init_module(laus_data* dataPtr);
int test_ioctl(laus_data* dataPtr);
int test_ioperm(laus_data* dataPtr);
int test_iopl(laus_data* dataPtr);
int test_kill(laus_data* dataPtr);
int test_lchown(laus_data* dataPtr);
int test_lchown16(laus_data* dataPtr);
int test_lchown32(laus_data* dataPtr);
int test_link(laus_data* dataPtr);
int test_lremovexattr(laus_data* dataPtr);
int test_lsetxattr(laus_data* dataPtr);
int test_mkdir(laus_data* dataPtr);
int test_mknod(laus_data* dataPtr);
int test_mount(laus_data* dataPtr);
int test_msgctl(laus_data* dataPtr);
int test_msgget(laus_data* dataPtr);
int test_msgrcv(laus_data* dataPtr);
int test_msgsnd(laus_data* dataPtr);
int test_open(laus_data* dataPtr);
int test_ptrace(laus_data* dataPtr);
int test_reboot(laus_data* dataPtr);
int test_removexattr(laus_data* dataPtr);
int test_rename(laus_data* dataPtr);
int test_rmdir(laus_data* dataPtr);
int test_semctl(laus_data* dataPtr);
int test_semget(laus_data* dataPtr);
int test_semop(laus_data* dataPtr);
int test_semtimedop(laus_data* dataPtr);
int test_setdomainname(laus_data* dataPtr);
int test_setfsgid(laus_data* dataPtr);
int test_setfsgid16(laus_data* dataPtr);
int test_setfsgid32(laus_data* dataPtr);
int test_setfsuid(laus_data* dataPtr);
int test_setfsuid16(laus_data* dataPtr);
int test_setfsuid32(laus_data* dataPtr);
int test_setgid(laus_data* dataPtr);
int test_setgid16(laus_data* dataPtr);
int test_setgid32(laus_data* dataPtr);
int test_setgroups(laus_data* dataPtr);
int test_setgroups16(laus_data* dataPtr);
int test_setgroups32(laus_data* dataPtr);
int test_sethostname(laus_data* dataPtr);
int test_setpgid(laus_data* dataPtr);
int test_setpriority(laus_data* dataPtr);
int test_setregid(laus_data* dataPtr);
int test_setregid16(laus_data* dataPtr);
int test_setregid32(laus_data* dataPtr);
int test_setresgid(laus_data* dataPtr);
int test_setresgid16(laus_data* dataPtr);
int test_setresgid32(laus_data* dataPtr);
int test_setresuid(laus_data* dataPtr);
int test_setresuid16(laus_data* dataPtr);
int test_setresuid32(laus_data* dataPtr);
int test_setreuid(laus_data* dataPtr);
int test_setreuid16(laus_data* dataPtr);
int test_setreuid32(laus_data* dataPtr);
int test_setrlimit(laus_data* dataPtr);
int test_setsid(laus_data* dataPtr);
int test_settimeofday(laus_data* dataPtr);
int test_setuid(laus_data* dataPtr);
int test_setuid16(laus_data* dataPtr);
int test_setuid32(laus_data* dataPtr);
int test_setxattr(laus_data* dataPtr);
int test_shmat(laus_data* dataPtr);
int test_shmctl(laus_data* dataPtr);
int test_shmdt(laus_data* dataPtr);
int test_shmget(laus_data* dataPtr);
int test_swapoff(laus_data* dataPtr);
int test_swapon(laus_data* dataPtr);
int test_symlink(laus_data* dataPtr);
int test_syslog(laus_data* dataPtr);
int test_tkill(laus_data* dataPtr);
int test_truncate(laus_data* dataPtr);
int test_truncate64(laus_data* dataPtr);
int test_umask(laus_data* dataPtr);
int test_unlink(laus_data* dataPtr);
int test_utime(laus_data* dataPtr);
int test_utimes(laus_data* dataPtr);
int test_vfork(laus_data* dataPtr);

#endif
