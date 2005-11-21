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
**  FILE   : filterconf.h
**
**  PURPOSE: This file is the main include file for the filter
**           configuration tests.  It provides various defines, typdefs
**           and function declarations.
**
**
**  HISTORY:
**    08/03 originated by Tom Lendacky (toml@us.ibm.com)
**    05/04 Adapted for EAL4 by Dan Jones (danjones@us.ibm.com)
**
**********************************************************************/

#ifndef _FILTERCONF_H
#define _FILTERCONF_H

#define DEVNAME_LEN		32

#define AUDIT_CONF		"/etc/audit/audit.conf"
#define FILTER_CONF		"/etc/audit/filter.conf"
#define FILTER_TEST_CONF	"/etc/audit/filter_test.conf"

#define SETRC_PROGRAM		"./utils/rc_return"
#define SETRC_RC		"10"

#define PROCESS_RC		10
#define TEST_GID                10

#define STR_PATH1		"/etc/audit"
#define STR_PATH2		"/home"

//#if defined(__s390__) || defined(__s390x__)
//#define ETHERNET_DEV            "iucv"
//#else
#define ETHERNET_DEV		"eth"
//#endif

#define ROUTE_ADD_CMD		"/sbin/ip -f inet route add 192.168.32.0/24 dev "ETHERNET_DEV"0"
#define ROUTE_DEL_CMD		"/sbin/ip -f inet route del 192.168.32.0/24"

#define GROUP_ADD_CMD		"/usr/sbin/groupadd -g 777 -o filterconfgroup"
#define GROUP_DEL_CMD		"/usr/sbin/groupdel filterconfgroup"
#define GROUP_ADD_MSG		"groupadd: group added - group=filterconfgroup, gid=777, by=%d"


// Function pointer definition for all filter configuration tests
typedef int (*filterconfFunctions)(laus_data *, uid_t, gid_t);

// Filter configuration test case data structure
typedef struct {
	filterconfFunctions	testPtr;
	char *			testName;
	laus_data *		dataPtr;
	log_options		expectedLogResult;
	char *			filter;
} filterconf_data;

int test_int_cmp(laus_data *ld, uid_t uid, gid_t gid);
int test_mask_cmp(laus_data *ld, uid_t uid, gid_t gid);
int test_str_cmp(laus_data *ld, uid_t uid, gid_t gid);
int test_syscall_argN(laus_data *ld, uid_t uid, gid_t gid);
int test_syscall_minor(laus_data *ld, uid_t uid, gid_t gid);
int test_process_login_uid(laus_data *ld, uid_t uid, gid_t gid);
int test_process_uid(laus_data *ld, uid_t uid, gid_t gid);
int test_process_gid(laus_data *ld, uid_t uid, gid_t gid);
int test_process_dumpable_on(laus_data *ld, uid_t uid, gid_t gid);
int test_process_dumpable_off(laus_data *ld, uid_t uid, gid_t gid);
int test_process_exit_normal(laus_data *ld, uid_t uid, gid_t gid);
int test_process_exit_signal(laus_data *ld, uid_t uid, gid_t gid);
int test_file(laus_data *ld, uid_t uid, gid_t gid);
int test_file_owner(laus_data *ld, uid_t uid, gid_t gid);
int test_dev(laus_data *ld, uid_t uid, gid_t gid);
int test_user_msg(laus_data *ld, uid_t uid, gid_t gid);
int test_login(laus_data *ld, uid_t uid, gid_t gid);

#endif
