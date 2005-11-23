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

struct syscall_opts {
    unsigned int help;
    unsigned int success;
    char	 testcase[MAX_TESTCASE_NAME + 1];
    char	 variation[MAX_VARIATION_NAME + 1];
};

/* global to syscalls tests */
int helper_uid;

static void usage()
{
    char *usage = "Usage:\n\
  syscalls -t <syscall_name> [-v <variation>] [-s]\n\n\
Arguments:\n\
  -h                  Display this message.\n\
  -t <syscall_name>   Specify name of system call to test.\n\
  -v <test_variation> Specify testcase variation.\n\
  -s                  Specify testcase expected success (yes when present).\n";
    fprintf(stderr, "%s\n", usage);
}

static int parse_command_line(int argc, char **argv, 
			      struct syscall_opts *options)
{
    int rc = 0;
    int opt;

    memset(options, 0, sizeof(struct syscall_opts));

    while ((opt = getopt(argc, argv, "t:v:hs")) != -1) {
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
	    case 'v':
		if (strlen(optarg) + 1 > MAX_VARIATION_NAME)
		    rc = -1;
		else
		    strncpy(options->variation, optarg, MAX_VARIATION_NAME);
		break;
	    default:
		rc = -1;
	}
    }

    if (rc == 0 && strcmp(options->testcase, "") == 0)
	    rc = -1;

    return rc;
}

static int lookup_variation(char *varstr)
{
    if (!varstr || (strcmp(varstr, "basic") == 0))
	return SYSCALL_BASIC;
    if (strcmp(varstr, "remove") == 0)
	return SYSCALL_REMOVE;
    if (strcmp(varstr, "setperms") == 0)
	return SYSCALL_SETPERMS;
    return -1;
}

int main(int argc, char **argv)
{
    int			rc = 0;
    ts_exit		ecode = TEST_EXPECTED;
    struct syscall_opts options;
    struct audit_data 	context;
    int			(*test_handle)(struct audit_data *, int, int);

    rc = parse_command_line(argc, argv, &options);
    if (rc || options.help) {
	ecode = TEST_ERROR;
	usage();
	goto exit;
    }

    context_init(&context, AUDIT_MSG_SYSCALL);
    rc = context_initsyscall(&context, options.testcase);
    if (rc) {
	ecode = TEST_ERROR;
	goto exit;
    }

    rc = lookup_testcase(&test_handle, options.testcase);
    if (rc) {
	ecode = TEST_ERROR;
	fprintf(stderr, "Error: test \"%s\" not found\n", options.testcase);
	goto exit;
    }

    rc = test_handle(&context, lookup_variation(options.variation), 
		     options.success);
    if (rc) {
	ecode = TEST_ERROR;
	fprintf(stderr, "Error: testcase unable to complete\n");
	goto exit;
    }

    ecode = verify_opresult(&context, options.success);
    if (ecode != TEST_EXPECTED)
	goto exit;

    ecode = verify_logresult(&context);

exit:
    context_release(&context);
    return ecode;
}
