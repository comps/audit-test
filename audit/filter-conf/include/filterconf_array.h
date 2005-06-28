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
**  FILE   : filterconf_array.h
**
**  PURPOSE: This file contains the array definition for all of the
**           filter configuration tests.
**
**
**  HISTORY:
**    08/03 originated by Tom Lendacky (toml@us.ibm.com)
**    05/04 Adapted for EAL4 by Dan Jones (danjones@us.ibm.com)
**
**********************************************************************/

#ifndef _FILTERCONF_ARRAY_H
#define _FILTERCONF_ARRAY_H

#include "filterconf.h"


// Based on tests being run as root
filterconf_data filterconf_tests[] = {
  	// Process attribute tests...
	// Process login-uid
        // BUGBUG: The test_process_dumpable tests must
        //         precede the the test_int_cmp test 
        //         because the setgid call in test_int_cmp
        //         will alter the dumpable flag.
	{ &test_process_login_uid, "process loginuid true",  NULL, { TRUE, TRUE },   "event process-exit = is-loginuid(login-uid);" },
	{ &test_process_login_uid, "process loginuid false", NULL, { FALSE, FALSE }, "event process-exit = is-not-loginuid(login-uid);" },
	// Process uid
	{ &test_process_uid, "process uid true",  NULL, { TRUE, TRUE },   "event process-exit = is-testuid(uid);" },
	{ &test_process_uid, "process uid false", NULL, { FALSE, FALSE }, "event process-exit = is-root(uid);" },
	// Process gid
	{ &test_process_gid, "process gid true",  NULL, { TRUE, TRUE },   "event process-exit = is-testgid(gid);" },
	{ &test_process_gid, "process gid false", NULL, { FALSE, FALSE }, "event process-exit = is-root(gid);" },
	// Process dumpable
	{ &test_process_dumpable_on,  "process dumpable true",  NULL, { TRUE, TRUE },   "event process-exit = is-not-null(dumpable);" },
	{ &test_process_dumpable_off, "process dumpable true",  NULL, { TRUE, TRUE },   "event process-exit = is-null(dumpable);" },
	{ &test_process_dumpable_on,  "process dumpable false", NULL, { FALSE, FALSE }, "event process-exit = is-null(dumpable);" },
	{ &test_process_dumpable_off, "process dumpable false", NULL, { FALSE, FALSE }, "event process-exit = is-not-null(dumpable);" },
	// Process exitcode
	{ &test_process_exit_normal, "process exitcode true",  NULL, { TRUE, TRUE },   "event process-exit = mask-normal(exitcode);" },
	{ &test_process_exit_normal, "process exitcode false", NULL, { FALSE, FALSE }, "event process-exit = mask-abrtsignal(exitcode);" },
	{ &test_process_exit_signal, "process exitcode true",  NULL, { TRUE, TRUE },   "event process-exit = mask-abrtsignal(exitcode);" },
	{ &test_process_exit_signal, "process exitcode false", NULL, { FALSE, FALSE }, "event process-exit = mask-normal(exitcode);" },

	// Integer comparison tests
	{ &test_int_cmp, "eq true",     NULL, { TRUE, TRUE },   "syscall setgid = eq-10(arg0);" },
	{ &test_int_cmp, "eq false",    NULL, { FALSE, FALSE }, "syscall setgid = eq-15(arg0);" },
	{ &test_int_cmp, "ne true",     NULL, { TRUE, TRUE },   "syscall setgid = ne-15(arg0);" },
	{ &test_int_cmp, "ne false",    NULL, { FALSE, FALSE }, "syscall setgid = ne-10(arg0);" },
	{ &test_int_cmp, "lt true",     NULL, { TRUE, TRUE },   "syscall setgid = lt-15(arg0);" },
	{ &test_int_cmp, "lt false",    NULL, { FALSE, FALSE }, "syscall setgid = lt-10(arg0);" },
	{ &test_int_cmp, "lt false",    NULL, { FALSE, FALSE }, "syscall setgid = lt-5(arg0);" },
	{ &test_int_cmp, "le true",     NULL, { TRUE, TRUE },   "syscall setgid = le-15(arg0);" },
	{ &test_int_cmp, "le true",     NULL, { TRUE, TRUE },   "syscall setgid = le-10(arg0);" },
	{ &test_int_cmp, "le false",    NULL, { FALSE, FALSE }, "syscall setgid = le-5(arg0);" },
	{ &test_int_cmp, "ge true",     NULL, { TRUE, TRUE },   "syscall setgid = ge-5(arg0);" },
	{ &test_int_cmp, "ge true",     NULL, { TRUE, TRUE },   "syscall setgid = ge-10(arg0);" },
	{ &test_int_cmp, "ge false",    NULL, { FALSE, FALSE }, "syscall setgid = ge-15(arg0);" },
	{ &test_int_cmp, "gt true",     NULL, { TRUE, TRUE },   "syscall setgid = gt-5(arg0);" },
	{ &test_int_cmp, "gt false",    NULL, { FALSE, FALSE }, "syscall setgid = gt-10(arg0);" },
	{ &test_int_cmp, "gt false",    NULL, { FALSE, FALSE }, "syscall setgid = gt-15(arg0);" },
	{ &test_mask_cmp, "mask true",  NULL, { TRUE, TRUE},    "event process-exit = mask-abrtsignal(exitcode);" },
	{ &test_mask_cmp, "mask false", NULL, { FALSE, FALSE }, "event process-exit = mask-normal(exitcode);" },

	// String comparison tests
	{ &test_str_cmp, "streq true",   NULL, { TRUE, TRUE },   "syscall open = streq-file1(arg0);" },
	{ &test_str_cmp, "streq false",  NULL, { FALSE, FALSE }, "syscall open = streq-file2(arg0);" },
	{ &test_str_cmp, "prefix true",  NULL, { TRUE, TRUE },   "syscall open = prefix-path1(arg0);" },
	{ &test_str_cmp, "prefix false", NULL, { FALSE, FALSE }, "syscall open = prefix-path2(arg0);" },

	// True and False tests
	{ &test_str_cmp, "true",   NULL, { TRUE, TRUE },   "syscall open = true;" },
	{ &test_str_cmp, "always", NULL, { TRUE, TRUE },   "syscall open = always;" },
	{ &test_str_cmp, "false",  NULL, { FALSE, FALSE }, "syscall open = false;" },
	{ &test_str_cmp, "never",  NULL, { FALSE, FALSE }, "syscall open = never;" },

	// Return tests
	{ &test_str_cmp, "return log",         NULL, { TRUE, TRUE },   "syscall open = return( log );" },
	{ &test_str_cmp, "return log-verbose", NULL, { TRUE, TRUE },   "syscall open = return( log-verbose );" },
	{ &test_str_cmp, "return ignore",      NULL, { FALSE, FALSE }, "syscall open = return( ignore );" },

	// Syscall related tests...
	//    (the syscall result tests are covered by the syscalls test)
	// Syscall argument tests using setreuid (ruid=testuser-uid, euid=0)
	{ &test_syscall_argN, "syscall arg0 true",  NULL, { TRUE, TRUE },   "syscall setreuid = is-testuid(arg0);" },
	{ &test_syscall_argN, "syscall arg0 false", NULL, { FALSE, FALSE }, "syscall setreuid = is-root(arg0);" },
	{ &test_syscall_argN, "syscall arg1 true",  NULL, { TRUE, TRUE },   "syscall setreuid = is-root(arg1);" },
	{ &test_syscall_argN, "syscall arg1 false", NULL, { FALSE, FALSE }, "syscall setreuid = is-testuid(arg1);" },

	// Syscall syscall-minor tests
#if !defined(__X86_64) && !defined(__IA64)
	//{ &test_syscall_minor, "syscall minor true",  NULL, { TRUE, TRUE },   "syscall socketcall = is-socket-minor(syscall-minor);" },
	//{ &test_syscall_minor, "syscall minor false", NULL, { FALSE, FALSE }, "syscall socketcall = is-bind-minor(syscall-minor);" },
#endif

	// File attribute tests...
	// File mode
	{ &test_file, "file mode true",  NULL, { TRUE, TRUE },   "syscall open = is-file(arg0);" },
	{ &test_file, "file mode false", NULL, { FALSE, FALSE }, "syscall open = is-dir(arg0);" },
	// File inode
	{ &test_file, "file ino true",  NULL, { TRUE, TRUE },   "syscall open = is-file-inoA(arg0);" },
	{ &test_file, "file ino false", NULL, { FALSE, FALSE }, "syscall open = is-file-inoB(arg0);" },
	// File device
	{ &test_file, "file dev true",  NULL, { TRUE, TRUE },   "syscall open = is-file-devA(arg0);" },
	{ &test_file, "file dev false", NULL, { FALSE, FALSE }, "syscall open = is-file-devB(arg0);" },
	// File UID
	{ &test_file_owner, "file uid true",   NULL, { TRUE, TRUE },   "syscall open = is-file-testuid(arg0);" },
	{ &test_file_owner, "file uid false",  NULL, { FALSE, FALSE }, "syscall open = is-file-rootuid(arg0);" },
	// File GID
	{ &test_file_owner, "file gid true",  NULL, { TRUE, TRUE },   "syscall open = is-file-testgid(arg0);" },
	{ &test_file_owner, "file gid false", NULL, { FALSE, FALSE }, "syscall open = is-file-rootgid(arg0);" },
	// Device major
	{ &test_dev, "file dev-major true",  NULL, { TRUE, TRUE },   "syscall open = is-dev-devmajorA(arg0);" },
	{ &test_dev, "file dev-major false", NULL, { FALSE, FALSE }, "syscall open = is-dev-devmajorB(arg0);" },
	// Device minor
	{ &test_dev, "file dev-minor true",  NULL, { TRUE, TRUE },   "syscall open = is-dev-devminorA(arg0);" },
	{ &test_dev, "file dev-minor false", NULL, { FALSE, FALSE }, "syscall open = is-dev-devminorB(arg0);" },

	// Socket attribute tests...
	// Socket family
	{ &test_socket, "socket family true",  NULL, { TRUE, TRUE },   "syscall bind = is-sock-inet(arg0);" },
	{ &test_socket, "socket family false", NULL, { FALSE, FALSE }, "syscall bind = is-sock-inet6(arg0);" },
	// Socket type
	{ &test_socket, "socket type true",  NULL, { TRUE, TRUE },   "syscall bind = is-sock-stream(arg0);" },
	{ &test_socket, "socket type false", NULL, { FALSE, FALSE }, "syscall bind = is-sock-dgram(arg0);" },

	// Netlink attribute tests...
	// Netlink family
	{ &test_netlink, "netlink family true",  NULL, { TRUE, TRUE },   "event network-config = is-inet(netlink-family);" },
	{ &test_netlink, "netlink family false", NULL, { FALSE, FALSE }, "event network-config = is-inet6(netlink-family);" },
	// Netlink type
	{ &test_netlink, "netlink type true",  NULL, { TRUE, TRUE },   "event network-config = is-nl-newroute(netlink-type);" },
	{ &test_netlink, "netlink type false", NULL, { FALSE, FALSE }, "event network-config = is-nl-newaddr(netlink-type);" },
	// Netlink flags
	{ &test_netlink, "netlink flags true",  NULL, { TRUE, TRUE },   "event network-config = is-nl-request(netlink-flags);" },
	{ &test_netlink, "netlink flags false", NULL, { FALSE, FALSE }, "event network-config = is-nl-norequest(netlink-flags);" },

	// Userspace tests...
	{ &test_user_msg, "user message true",  NULL, { TRUE, TRUE },   "event user-message = always;" },
	{ &test_user_msg, "user message false", NULL, { FALSE, FALSE }, "event user-message = never;" },

	// Login tests...
	{ &test_login, "login true",  NULL, { TRUE, TRUE },   "event process-login = is-root(login-uid);" },
	{ &test_login, "login false", NULL, { FALSE, FALSE }, "event process-login = is-testuid(login-uid);" },

	// Set tests...
	//    (uses of the test_str_cmp test)
	{ &test_str_cmp, "set true",  NULL, { TRUE, TRUE },   "syscall open = is-dir-set1(arg0);" },
	{ &test_str_cmp, "set true",  NULL, { TRUE, TRUE },   "syscall open = is-dir-set2(arg0);" },
	{ &test_str_cmp, "set true",  NULL, { TRUE, TRUE },   "syscall open = is-dir-set3(arg0);" },
	{ &test_str_cmp, "set false", NULL, { FALSE, FALSE }, "syscall open = is-dir-set4(arg0);" },

	// Boolean operators tests...
	//    (uses of the test_syscall_argN test)
	// && operator
	{ &test_syscall_argN, "boolean true  && true  = true",  NULL, { TRUE, TRUE },   "syscall setreuid = is-testuid(arg0) && is-root(arg1);" },
	{ &test_syscall_argN, "boolean true  && false = false", NULL, { FALSE, FALSE }, "syscall setreuid = is-testuid(arg0) && is-testuid(arg1);" },
	{ &test_syscall_argN, "boolean false && true  = false", NULL, { FALSE, FALSE }, "syscall setreuid = is-root(arg0) && is-root(arg1);" },
	{ &test_syscall_argN, "boolean false && false = false", NULL, { FALSE, FALSE }, "syscall setreuid = is-root(arg0) && is-testuid(arg1);" },
	// || operator
	{ &test_syscall_argN, "boolean true  || true  = true",  NULL, { TRUE, TRUE },   "syscall setreuid = is-testuid(arg0) || is-root(arg1);" },
	{ &test_syscall_argN, "boolean true  || false = true",  NULL, { TRUE, TRUE },   "syscall setreuid = is-testuid(arg0) || is-testuid(arg1);" },
	{ &test_syscall_argN, "boolean false || true  = true",  NULL, { TRUE, TRUE },   "syscall setreuid = is-root(arg0) || is-root(arg1);" },
	{ &test_syscall_argN, "boolean false || false = false", NULL, { FALSE, FALSE }, "syscall setreuid = is-root(arg0) || is-testuid(arg1);" },
	// ! operator
	{ &test_syscall_argN, "boolean !false = true",  NULL, { TRUE, TRUE },   "syscall setreuid = !is-root(arg0);" },
	{ &test_syscall_argN, "boolean !true  = false", NULL, { FALSE, FALSE }, "syscall setreuid = !is-testuid(arg0);" },
};

#endif
