/*  Copyright (C) International Business Machines  Corp., 2003
 *  (c) Copyright Hewlett-Packard Development Company, L.P., 2005
 *
 *  This program is free software;  you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY;  without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See
 *  the GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program;  if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 *
 *  Implementation written by HP, based on original code from IBM.
 *
 *  FILE:
 *  test_init_module.c
 *
 *  PURPOSE:
 *  Verify audit of attempts to initialize loadable kernel modules.
  *
 *  SYSCALLS:
 *  init_module()
 *
 *  TESTCASE: successful
 *  Load kernel module as root user.
 *
 *  TESTCASE: unsuccessful
 *  Attempt to load kernel module as test user.
 */

#include "includes.h"
#include "syscalls.h"
#include <sys/mman.h>

int test_init_module(struct audit_data *context)
{
    int rc = 0;
    int success = context->success; /* save intended result */
    char module_path[256] = { 0 };
    int fd;
    struct stat mstat;
    void *region;
    int exit;
    char *module_name = "test_module";

    /* set up test kernel module */
    sprintf(module_path, "%s/test_module.ko", getenv("AUDIT_MODULE_PATH") ?: ".");
    fprintf(stderr, "Module path: %s\n", module_path);
    fd = open(module_path, O_RDONLY);
    if (fd < 0) {
	fprintf(stderr, "Error: opening module path %s: %s\n", 
		module_path, strerror(errno));
	return -1;
    }
    rc = fstat(fd, &mstat);
    if (rc < 0) {
	rc = -1;
	fprintf(stderr, "Error: getting module file size: %s\n", strerror(errno));
	goto exit;
    }
    region = mmap(NULL, mstat.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
    if (region == MAP_FAILED) {
	rc = -1;
	fprintf(stderr, "Error: mmap(): %s\n", strerror(errno));
	goto exit;
    }

    /* To produce failure, attempt to load module as test user */
    if (!success) {
	rc = seteuid_test();
	if (rc < 0)
	    goto exit_mem;
	context->experror = EPERM;
    }

    rc = context_setidentifiers(context);
    if (rc < 0)
	goto exit_mem;

    errno = 0;
    context_setbegin(context);
    exit = syscall(context->u.syscall.sysnum, region, mstat.st_size, "");
    context_setend(context);

    if (exit < 0) {
	context->success = 0;
	context->u.syscall.exit = context->error = errno;
    } else {
	context->success = 1;
	context->u.syscall.exit = exit;

	rc = syscall(__NR_delete_module, module_name, 0);
	if (rc < 0)
	    fprintf(stderr, "Error removing module: %s: %s\n",
		    module_name, strerror(errno));
    }

exit_mem:
    if (!success)
	seteuid(0); /* preserve potential error from delete_module */
    munmap(region, mstat.st_size);

exit:
    close(fd);
    return rc;
}
