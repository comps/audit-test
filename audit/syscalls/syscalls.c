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

    if (rc == 0 && 
	((!strcmp(options->testcase, "") || !strcmp(options->variation, ""))))
	    rc = -1;

    return rc;
}

int main(int argc, char **argv)
{
    ts_exit		ecode = TEST_EXPECTED;
    struct syscall_opts options;
    struct audit_data 	context;
    int			(*test_handle)(struct audit_data *, int, int);
    int			varnum;

    if ((parse_command_line(argc, argv, &options) < 0) || options.help) {
	ecode = TEST_ERROR;
	usage();
	goto exit;
    }

    context_init(&context, AUDIT_MSG_SYSCALL);
    if (context_initsyscall(&context, options.testcase) < 0) {
	ecode = TEST_ERROR;
	goto exit;
    }

    test_handle = lookup_testcase(options.testcase);
    varnum = lookup_variation(options.variation);
    if (!test_handle || (varnum < 0)) {
	ecode = TEST_ERROR;
	goto exit;
    }

    if (test_handle(&context, varnum, options.success) < 0) {
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
