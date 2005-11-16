/**********************************************************************
**   Copyright (C) International Business Machines  Corp., 2003
**   (c) Copyright Hewlett-Packard Development Company, L.P., 2005
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
**   Implementation written by HP, based on original code from IBM.
** 
**********************************************************************/

#include "includes.h"
#include "syscalls.h"

#include <libaudit.h>
#include <pwd.h>

struct syscall_opts {
    unsigned int help;
    unsigned int success;
    char	 testcase[MAX_TESTCASE_NAME + 1];
};

/* global to syscalls tests */
int helper_uid;

static void usage()
{
    char *usage = "Usage:\n\
  syscalls -t <syscall_name> [-s]\n\n\
Arguments:\n\
  -h                  Display this message.\n\
  -t <syscall_name>   Specify name of system call to test.\n\
  -s                  Specify testcase expected success (yes when present)\n";
    fprintf(stderr, "%s\n", usage);
}

static int parse_command_line(int argc, char **argv, 
			      struct syscall_opts *options)
{
    int rc = 0;
    int opt;

    memset(options, 0, sizeof(struct syscall_opts));

    while ((opt = getopt(argc, argv, "u:t:hs")) != -1) {
	switch(opt) {
	    case 'h':
		options->help = 1;
		break;
	    case 's':
		options->success = 1;
		break;
	    case 't':
		if (strlen(optarg) + 1 > MAX_TESTCASE_NAME)
		    rc = -1;
		else
		    strncpy(options->testcase, optarg, MAX_TESTCASE_NAME);
		break;
	    default:
		rc = -1;
	}
    }

    if (rc == 0 && strcmp(options->testcase, "") == 0)
	    rc = -1;

    return rc;
}

static int init_context(struct audit_data *context, char *testname, 
			unsigned int success)
{
    int rc = 0;
    int loginuid;
    int sysnum;
    struct passwd *pw = NULL;
    char cwd[PATH_MAX];

    sysnum = audit_name_to_syscall(testname, audit_detect_machine());
    if (sysnum < 0) {
	rc = -1;
	fprintf(stderr, "Error: unable to translate \"%s\" to syscall number\n",
		testname);
	goto exit;
    }

    loginuid = getLoginUID();
    if (loginuid < 0) {
	rc = -1;
	fprintf(stderr, "Error: unable to get login uid\n");
	goto exit;
    }

    pw = getpwnam(TEST_USER);
    if (!pw) {
	rc = -1;
	fprintf(stderr, "Error: unable to get passwd info for %s\n", TEST_USER);
	goto exit;
    }

    errno = 0;
    if (getcwd(cwd, PATH_MAX) == NULL) {
	rc = -1;
	fprintf(stderr, 
		"Error: unable to get current working directory: %s\n",
		strerror(errno));
	goto exit;
    }

    memset(context, 0, sizeof(struct audit_data));
    context->type    = AUDIT_MSG_SYSCALL;
    context->uid     = pw->pw_uid;
    context->euid    = pw->pw_uid;
    context->fsuid   = pw->pw_uid;
    context->gid     = pw->pw_gid;
    context->egid    = pw->pw_gid;
    context->fsgid   = pw->pw_gid;
    context->success = success;
    context->u.syscall.arch   = TS_BUILD_ARCH;
    context->u.syscall.sysnum = sysnum;
    strncpy(context->u.syscall.cwd, cwd, PATH_MAX);

exit:
    return rc;
}

int main(int argc, char **argv)
{
    int			rc = 0;
    ts_exit		ecode = TEST_EXPECTED;
    struct syscall_opts options;
    struct audit_data 	context;
    int			(*test_handle)(struct audit_data *);

    rc = parse_command_line(argc, argv, &options);
    if (rc || options.help) {
	ecode = TEST_ERROR;
	usage();
	goto exit;
    }

    fprintf(stderr, "\nBegin test [%s: %s]\n", options.testcase, 
	    options.success ? "good" : "bad");

    rc = init_context(&context, options.testcase, options.success);
    if (rc) {
	ecode = TEST_ERROR;
	fprintf(stderr, "Error: unable to initialize syscall context\n");
	goto exit;
    }

    rc = lookup_testcase(&test_handle, options.testcase);
    if (rc) {
	ecode = TEST_ERROR;
	fprintf(stderr, "Error: test [%s] not found\n", options.testcase);
	goto exit;
    }

    rc = test_handle(&context);
    if (rc) {
	ecode = TEST_ERROR;
	fprintf(stderr, "Error: test [%s: %s] unable to complete\n", 
		options.testcase, options.success ? "good" : "bad");
	goto exit;
    }

    ecode = verify_opresult(&context, options.success);
    if (ecode != TEST_EXPECTED)
	goto exit;

    /* 1 = always want to find log record matching specified context */
    ecode = verify_logresult(&context, 1);

    fprintf(stderr, "End test [%s: %s]\n", options.testcase, 
	    options.success ? "good" : "bad");

    /* TODO free any addtl context memory */

exit:
    return ecode;
}
