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
 *  test_delete_module.c
 *
 *  PURPOSE:
 *  Verify audit of attempts to delete kernel module entries.
  *
 *  SYSCALLS:
 *  delete_module()
 *
 *  TESTCASE: successful
 *  Delete module entry as root user.
 *
 *  TESTCASE: unsuccessful
 *  Attempt to delete module entry as test user.
 */

#include "includes.h"
#include "syscalls.h"
#include <sys/mman.h>

int test_delete_module(struct audit_data *context)
{
    int rc = 0;
    int success = context->success; /* save intended result */
    char module_path[256] = { 0 };
    int fd;
    struct stat mstat;
    void *region;
    int exit = 0;
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

    /* load module, so we can attempt to remove it */
    errno = 0;
    rc = syscall(__NR_init_module, region, mstat.st_size, "");
    if (rc < 0) {
	fprintf(stderr, "Error: loading module: %s\n", strerror(errno));
	goto exit_mem;
    }
    fprintf(stderr, "Loaded module: %s\n", module_path);

    /* To produce failure, attempt to delete module entry as test user */
    if (!success) {
	rc = seteuid_test();
	if (rc < 0)
	    goto exit_mod;
	context->experror = EPERM;
    }

    rc = context_setidentifiers(context);
    if (rc < 0)
	goto exit_mod;

    errno = 0;
    context_setbegin(context);
    exit = syscall(context->u.syscall.sysnum, module_name, 0);
    context_setend(context);

    if (exit < 0) {
	context->success = 0;
	context->u.syscall.exit = context->error = errno;
    } else {
	context->success = 1;
	context->u.syscall.exit = exit;
    }

exit_mod:
    if (!success)
	seteuid(0);

    if (exit < 0) {
	rc = syscall(__NR_delete_module, module_name, 0);
	if (rc < 0)
	    fprintf(stderr, "Error removing module: %s: %s\n",
		    module_name, strerror(errno));
    }

exit_mem:
    munmap(region, mstat.st_size);

exit:
    close(fd);
    return rc;
}
