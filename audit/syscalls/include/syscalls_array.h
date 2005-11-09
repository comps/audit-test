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
**  FILE   : syscalls_array.h 
**
**  PURPOSE: This file contains the array definition for all of the
**           system calls to be tested, as well as architecture
**           specific logic.
**
**
**  HISTORY:
**    05/03 originated by Dan Jones (danjones@us.ibm.com)
**    06/03 furthered by Dustin Kirkland (k1rkland@us.ibm.com
**
**********************************************************************/

/*
 * Syscall Test Matrix
 */
typedef struct {
    int         (*testPtr)(laus_data *);
    char        *testName;
} syscall_data;

syscall_data syscallTests[] = {
    { &test_access, "access" }, 
    { &test_adjtimex, "adjtimex" }, 
    { &test_bind, "bind" }, 
    { &test_capset, "capset" }, 
    { &test_chdir, "chdir" }, 
    { &test_chmod, "chmod" }, 
    { &test_chown, "chown" }, 
#if !defined(__PPC) && !defined(__X86_64) && !defined(__S390X) && !defined(__IA64)
    { &test_chown16, "chown16" }, 
    { &test_chown32, "chown32" }, 
#endif
    { &test_chroot, "chroot" }, 
    { &test_clone, "clone" }, 
#ifdef __IA64
    { &test_clone2, "clone2" }, 
#endif
    { &test_creat, "creat" }, 
    { &test_delete_module, "delete_module" }, 
    { &test_execve, "execve" }, 
    { &test_fchdir, "fchdir" }, 
    { &test_fchmod, "fchmod" }, 
    { &test_fchown, "fchown" }, 
#if !defined(__PPC) && !defined(__X86_64) && !defined(__S390X) && !defined(__IA64)
    { &test_fchown16, "fchown16" }, 
    { &test_fchown32, "fchown32" }, 
#endif
    { &test_fork, "fork" }, 
    { &test_fremovexattr, "fremovexattr" }, 
    { &test_fsetxattr, "fsetxattr" }, 
    { &test_ftruncate, "ftruncate" }, 
#if !defined(__PPC) && !defined(__X86_64) && !defined(__S390X) && !defined(__IA64)
    { &test_ftruncate64, "ftruncate64" }, 
#endif
    { &test_init_module, "init_module" }, 
    { &test_ioctl, "ioctl" },
#ifdef __IX86
    { &test_ioperm, "ioperm" }, 
#endif
#ifdef __IX86
    { &test_iopl, "iopl" }, 
#endif
    { &test_kill, "kill" }, 
    { &test_lchown, "lchown" }, 
#if !defined(__PPC) && !defined(__X86_64) && !defined(__S390X) && !defined(__IA64)
    { &test_lchown16, "lchown16" }, 
    { &test_lchown32, "lchown32" }, 
#endif
    { &test_link, "link" }, 
    { &test_lremovexattr, "lremovexattr" }, 
    { &test_lsetxattr, "lsetxattr" }, 
    { &test_mkdir, "mkdir" }, 
    { &test_mknod, "mknod" }, 
    { &test_mount, "mount" }, 
    { &test_msgctl, "msgctl" }, 
    { &test_msgsnd, "msgsnd" },
    { &test_msgrcv, "msgrcv" },
    { &test_msgget, "msgget" }, 
    { &test_open,  "open" },
    { &test_ptrace,  "ptrace" }, 
    { &test_reboot, "reboot" }, 
    { &test_removexattr, "removexattr" }, 
    { &test_rename, "rename" }, 
    { &test_rmdir, "rmdir" }, 
    { &test_semctl, "semctl" }, 
    { &test_semget, "semget" }, 
    { &test_semop, "semop" },
    { &test_semtimedop, "semtimedop" },
    { &test_setdomainname, "setdomainname" }, 
    { &test_setfsgid, "setfsgid" }, 
#if !defined(__PPC) && !defined(__X86_64) && !defined(__S390X) && !defined(__IA64)
    { &test_setfsgid16, "setfsgid16" }, 
    { &test_setfsgid32, "setfsgid32" }, 
#endif
    { &test_setfsuid, "setfsuid" }, 
#if !defined(__PPC) && !defined(__X86_64) && !defined(__S390X) && !defined(__IA64)
    { &test_setfsuid16, "setfsuid16" }, 
    { &test_setfsuid32, "setfsuid32" }, 
#endif
    { &test_setgid, "setgid" }, 
#if !defined(__PPC) && !defined(__X86_64) && !defined(__S390X) && !defined(__IA64)
    { &test_setgid16, "setgid16" }, 
    { &test_setgid32, "setgid32" }, 
#endif
    { &test_setgroups, "setgroups" }, 
#if !defined(__PPC) && !defined(__X86_64) && !defined(__S390X) && !defined(__IA64)
    { &test_setgroups16, "setgroups16" }, 
    { &test_setgroups32, "setgroups32" }, 
#endif
    { &test_sethostname, "sethostname" }, 
    { &test_setpgid, "setpgid" }, 
    { &test_setpriority, "setpriority" }, 
    { &test_setregid, "setregid" }, 
#if !defined(__PPC) && !defined(__X86_64) && !defined(__S390X) && !defined(__IA64)
    { &test_setregid16, "setregid16" },
    { &test_setregid32, "setregid32" }, 
#endif
    { &test_setresgid, "setresgid" }, 
#if !defined(__PPC) && !defined(__X86_64) && !defined(__S390X) && !defined(__IA64)
    { &test_setresgid16, "setresgid16" }, 
    { &test_setresgid32, "setresgid32" }, 
#endif
    { &test_setresuid, "setresuid" }, 
#if !defined(__PPC) && !defined(__X86_64) && !defined(__S390X) && !defined(__IA64)
    { &test_setresuid16, "setresuid16" }, 
    { &test_setresuid32, "setresuid32" }, 
#endif
    { &test_setreuid, "setreuid" }, 
#if !defined(__PPC) && !defined(__X86_64) && !defined(__S390X) && !defined(__IA64)
    { &test_setreuid16, "setreuid16" }, 
    { &test_setreuid32, "setreuid32" }, 
#endif
    { &test_setrlimit, "setrlimit" }, 
    { &test_setsid, "setsid" }, 
    { &test_settimeofday, "settimeofday" }, 
    { &test_setuid, "setuid" }, 
#if !defined(__PPC) && !defined(__X86_64) && !defined(__S390X) && !defined(__IA64)
    { &test_setuid16, "setuid16" }, 
    { &test_setuid32, "setuid32" }, 
#endif
    { &test_setxattr, "setxattr" }, 
    { &test_shmat, "shmat" }, 
    { &test_shmctl, "shmctl" }, 
    { &test_shmdt, "shmdt" }, 
    { &test_shmget, "shmget" }, 
    { &test_swapoff, "swapoff" }, 
    { &test_swapon, "swapon" }, 
    { &test_symlink, "symlink" }, 
    { &test_syslog, "syslog" },  
    { &test_tkill, "tkill" },
    { &test_truncate, "truncate" }, 
#if !defined(__PPC) && !defined(__X86_64) && !defined(__S390X) && !defined(__IA64)
    { &test_truncate64, "truncate64" }, 
#endif
    { &test_umask, "umask" }, 
    { &test_unlink, "unlink" }, 
#ifndef __IA64
    { &test_utime, "utime" }, 
#endif
#ifdef __IA64
    { &test_utimes, "utimes" }, 
#endif
#ifndef __IA64
    { &test_vfork, "vfork" }, 
#endif
};
