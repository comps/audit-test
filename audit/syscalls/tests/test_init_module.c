/**********************************************************************
 **   Copyright (C) International Business Machines  Corp., 2003
 **   Copyright (C) 2005 Hewlett-Packard Company
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
 **   Contributors:
 **   2003  Michael A. Halcrow <mike@halcrow.us>
 **   2004  D. Kent Soper <dksoper@us.ibm.com>
 **   2005  Amy Griffis <amy.griffis@hp.com>
 **
 **********************************************************************/

#include "includes.h"
#include "syscalls.h"
#include <sys/mman.h>

extern long init_module(void *, unsigned long, const char *);
extern long delete_module(const char *, unsigned int);

int test_init_module(struct audit_data *context)
{

    int rc = 0;
    int exp_errno = EPERM;
    char *module_name = "test_module";
    char *module_path = "/tmp/laus_test/syscalls/kmods/test_module.ko";
    int flags = 0;
    int fd;
    struct stat mstat;
    void *region = MAP_FAILED;
    char dummy[] = { 0 };

    if (context->success) {
	context->euid = 0;
	context->egid = 0;
    }
    // Set up
    if ((fd = open(module_path, O_RDONLY)) == -1) {
	rc = fd;
	printf1("Error opening %s: %s [%d]\n", module_path,
		strerror(errno), errno);
	goto out;
    }

    if ((rc = fstat(fd, &mstat)) == -1) {
	printf1("Error getting module file size: %s [%d]\n",
		strerror(errno), errno);
	goto out;
    }

    region = mmap(NULL, mstat.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
    if (region == MAP_FAILED) {
	printf1("mmap:  %s [%d]\n", strerror(errno), errno);
	goto out;
    }
    // Set up audit argument buffer
    if ((rc = auditArg3(context,
			AUDIT_ARG_POINTER, 0, dummy,
			AUDIT_ARG_IMMEDIATE, sizeof(int), &mstat.st_size,
			AUDIT_ARG_STRING, 0, "")) != 0) {
	printf1("Error setting up audit argument buffer\n");
	goto out;
    }
    // Do pre-system call work
    if ((rc = preSysCall(context)) != 0) {
	printf1("ERROR: pre-syscall setup failed (%d)\n", rc);
	goto out;
    }
    // Execute system call
    context->u.syscall.exit = init_module(region, mstat.st_size, "");

    postSysCall(context, errno, -1, exp_errno);

out:
    if ((delete_module(module_name, flags) != 0) && (errno != ENOENT)) {
	printf1("Error removing module [%s]: %s [%d]\n", module_name,
		strerror(errno), errno);
    }
    if ((region != MAP_FAILED) && (munmap(region, mstat.st_size) != 0)) {
	printf1("munmap: %s [%d]\n", strerror(errno), errno);
    }
    if (fd != -1) {
	close(fd);
    }

    printf5("Returning from test\n");

    return rc;
}
