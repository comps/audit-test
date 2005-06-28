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

  syscall_data syscallTests[] = {
    { &test_access, "access", NULL }, 
    { &test_adjtimex, "adjtimex", NULL }, 
    { &test_bind, "bind", NULL }, 
    { &test_capset, "capset", NULL }, 
    { &test_chdir, "chdir", NULL }, 
    { &test_chmod, "chmod", NULL }, 
    { &test_chown, "chown", NULL }, 
#if !defined(__PPC) && !defined(__X86_64) && !defined(__S390X) && !defined(__IA64)
    { &test_chown16, "chown16", NULL }, 
    { &test_chown32, "chown32", NULL }, 
#endif
    { &test_chroot, "chroot", NULL }, 
    { &test_clone, "clone", NULL }, 
#ifdef __IA64
    { &test_clone2, "clone2", NULL }, 
#endif
    { &test_creat, "creat", NULL }, 
    { &test_delete_module, "delete_module", NULL }, 
    { &test_execve, "execve", NULL }, 
    { &test_fchdir, "fchdir", NULL }, 
    { &test_fchmod, "fchmod", NULL }, 
    { &test_fchown, "fchown", NULL }, 
#if !defined(__PPC) && !defined(__X86_64) && !defined(__S390X) && !defined(__IA64)
    { &test_fchown16, "fchown16", NULL }, 
    { &test_fchown32, "fchown32", NULL }, 
#endif
    { &test_fork, "fork", NULL }, 
    { &test_fremovexattr, "fremovexattr", NULL }, 
    { &test_fsetxattr, "fsetxattr", NULL }, 
    { &test_ftruncate, "ftruncate", NULL }, 
#if !defined(__PPC) && !defined(__X86_64) && !defined(__S390X) && !defined(__IA64)
    { &test_ftruncate64, "ftruncate64", NULL }, 
#endif
    { &test_init_module, "init_module", NULL }, 
    { &test_ioctl, "ioctl", NULL },
    /*
    { &test_ioctl_SIOCETHTOOL, "SIOCETHTOOL", NULL },
    { &test_ioctl_SIOCSIFLINK, "SIOCSIFLINK", NULL },

    { &test_ioctl_SIOCSIFPFLAGS, "SIOCSIFPFLAGS", NULL },
    { &test_ioctl_SIOCSMIIREG, "SIOCSMIIREG", NULL },
    { &test_ioctl_SIOCDIFADDR, "SIOCDIFADDR", NULL },
    { &test_ioctl_SIOCDRARP, "SIOCDRARP", NULL },
    { &test_ioctl_SIOCCHGTUNNEL, "SIOCCHGTUNNEL", NULL },
    { &test_ioctl_SIOCDELTUNNEL, "SIOCDELTUNNEL", NULL },
    { &test_ioctl_SIOCADDTUNNEL, "SIOCADDTUNNEL", NULL },
    { &test_ioctl_SIOCSIFFLAGS, "SIOCSIFFLAGS", NULL },
    { &test_ioctl_SIOCSIFADDR, "SIOCSIFADDR", NULL },
    { &test_ioctl_SIOCSIFDSTADDR, "SIOCSIFDSTADDR", NULL },
    { &test_ioctl_SIOCSIFBRDADDR, "SIOCSIFBRDADDR", NULL },
    { &test_ioctl_SIOCSIFNETMASK, "SIOCSIFNETMASK", NULL },
    { &test_ioctl_SIOCSIFMTU, "SIOCSIFMTU", NULL },
    { &test_ioctl_SIOCSIFHWADDR, "SIOCSIFHWADDR", NULL },
    { &test_ioctl_SIOCSIFTXQLEN, "SIOCSIFTXQLEN", NULL },
    { &test_ioctl_SIOCSIFHWBROADCAST, "SIOCSIFHWBROADCAST", NULL },
    { &test_ioctl_SIOCSARP, "SIOCSARP", NULL },
    { &test_ioctl_SIOCDARP, "SIOCDARP", NULL },
    { &test_ioctl_SIOCADDRT, "SIOCADDRT", NULL },
    { &test_ioctl_SIOCDELRT, "SIOCDELRT", NULL },
    { &test_ioctl_SIOCSIFNAME, "SIOCSIFNAME", NULL },
    { &test_ioctl_SIOCSIFMEM, "SIOCSIFMEM", NULL },
    { &test_ioctl_SIOCSIFSLAVE, "SIOCSIFSLAVE", NULL },
    { &test_ioctl_SIOCADDMULTI, "SIOCADDMULTI", NULL },
    { &test_ioctl_SIOCDELMULTI, "SIOCDELMULTI", NULL },
    { &test_ioctl_SIOCSIFMETRIC, "SIOCSIFMETRIC", NULL },
    { &test_ioctl_SIOCSIFMAP, "SIOCSIFMAP", NULL },
    { &test_ioctl_SIOCSIFBR, "SIOCSIFBR", NULL },
    { &test_ioctl_SIOCSIFENCAP, "SIOCSIFENCAP", NULL },
    */
#ifdef __IX86
    { &test_ioperm, "ioperm", NULL }, 
#endif
#ifdef __IX86
    { &test_iopl, "iopl", NULL }, 
#endif
    { &test_kill, "kill", NULL }, 
    { &test_lchown, "lchown", NULL }, 
#if !defined(__PPC) && !defined(__X86_64) && !defined(__S390X) && !defined(__IA64)
    { &test_lchown16, "lchown16", NULL }, 
    { &test_lchown32, "lchown32", NULL }, 
#endif
    { &test_link, "link", NULL }, 
    { &test_lremovexattr, "lremovexattr", NULL }, 
    { &test_lsetxattr, "lsetxattr", NULL }, 
    { &test_mkdir, "mkdir", NULL }, 
    { &test_mknod, "mknod", NULL }, 
    { &test_mount, "mount", NULL }, 
    { &test_msgctl, "msgctl", NULL }, 
    { &test_msgsnd, "msgsnd", NULL },
    { &test_msgrcv, "msgrcv", NULL },
    { &test_msgget, "msgget", NULL }, 
    { &test_open,  "open", NULL },
    { &test_ptrace,  "ptrace", NULL }, 
    { &test_reboot, "reboot", NULL }, 
    { &test_removexattr, "removexattr", NULL }, 
    { &test_rename, "rename", NULL }, 
    { &test_rmdir, "rmdir", NULL }, 
    { &test_semctl, "semctl", NULL }, 
    { &test_semget, "semget", NULL }, 
    { &test_semop, "semop", NULL },
    { &test_semtimedop, "semtimedop", NULL },
    { &test_setdomainname, "setdomainname", NULL }, 
    { &test_setfsgid, "setfsgid", NULL }, 
#if !defined(__PPC) && !defined(__X86_64) && !defined(__S390X) && !defined(__IA64)
    { &test_setfsgid16, "setfsgid16", NULL }, 
    { &test_setfsgid32, "setfsgid32", NULL }, 
#endif
    { &test_setfsuid, "setfsuid", NULL }, 
#if !defined(__PPC) && !defined(__X86_64) && !defined(__S390X) && !defined(__IA64)
    { &test_setfsuid16, "setfsuid16", NULL }, 
    { &test_setfsuid32, "setfsuid32", NULL }, 
#endif
    { &test_setgid, "setgid", NULL }, 
#if !defined(__PPC) && !defined(__X86_64) && !defined(__S390X) && !defined(__IA64)
    { &test_setgid16, "setgid16", NULL }, 
    { &test_setgid32, "setgid32", NULL }, 
#endif
    { &test_setgroups, "setgroups", NULL }, 
#if !defined(__PPC) && !defined(__X86_64) && !defined(__S390X) && !defined(__IA64)
    { &test_setgroups16, "setgroups16", NULL }, 
    { &test_setgroups32, "setgroups32", NULL }, 
#endif
    { &test_sethostname, "sethostname", NULL }, 
    { &test_setpgid, "setpgid", NULL }, 
    { &test_setpriority, "setpriority", NULL }, 
    { &test_setregid, "setregid", NULL }, 
#if !defined(__PPC) && !defined(__X86_64) && !defined(__S390X) && !defined(__IA64)
    { &test_setregid16, "setregid16", NULL },
    { &test_setregid32, "setregid32", NULL }, 
#endif
    { &test_setresgid, "setresgid", NULL }, 
#if !defined(__PPC) && !defined(__X86_64) && !defined(__S390X) && !defined(__IA64)
    { &test_setresgid16, "setresgid16", NULL }, 
    { &test_setresgid32, "setresgid32", NULL }, 
#endif
    { &test_setresuid, "setresuid", NULL }, 
#if !defined(__PPC) && !defined(__X86_64) && !defined(__S390X) && !defined(__IA64)
    { &test_setresuid16, "setresuid16", NULL }, 
    { &test_setresuid32, "setresuid32", NULL }, 
#endif
    { &test_setreuid, "setreuid", NULL }, 
#if !defined(__PPC) && !defined(__X86_64) && !defined(__S390X) && !defined(__IA64)
    { &test_setreuid16, "setreuid16", NULL }, 
    { &test_setreuid32, "setreuid32", NULL }, 
#endif
    { &test_setrlimit, "setrlimit", NULL }, 
    { &test_setsid, "setsid", NULL }, 
    { &test_settimeofday, "settimeofday", NULL }, 
    { &test_setuid, "setuid", NULL }, 
#if !defined(__PPC) && !defined(__X86_64) && !defined(__S390X) && !defined(__IA64)
    { &test_setuid16, "setuid16", NULL }, 
    { &test_setuid32, "setuid32", NULL }, 
#endif
    { &test_setxattr, "setxattr", NULL }, 
    { &test_shmat, "shmat", NULL }, 
    { &test_shmctl, "shmctl", NULL }, 
    { &test_shmdt, "shmdt", NULL }, 
    { &test_shmget, "shmget", NULL }, 
    { &test_swapoff, "swapoff", NULL }, 
    { &test_swapon, "swapon", NULL }, 
    { &test_symlink, "symlink", NULL }, 
    { &test_syslog, "syslog", NULL },  
    { &test_tkill, "tkill", NULL },
    { &test_truncate, "truncate", NULL }, 
#if !defined(__PPC) && !defined(__X86_64) && !defined(__S390X) && !defined(__IA64)
    { &test_truncate64, "truncate64", NULL }, 
#endif
    { &test_umask, "umask", NULL }, 
    { &test_unlink, "unlink", NULL }, 
#ifndef __IA64
    { &test_utime, "utime", NULL }, 
#endif
#ifdef __IA64
    { &test_utimes, "utimes", NULL }, 
#endif
#ifndef __IA64
    { &test_vfork, "vfork", NULL }, 
#endif
  };

