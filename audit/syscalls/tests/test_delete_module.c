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

int test_delete_module(struct audit_data *context, int variation, int success)
{
    int rc = 0;
    char module_path[256] = { 0 };
    char module_name[20] = { 0 };
    int fd;
    struct stat mstat;
    void *region;
    int exit = 0;

    strncpy(module_name, getenv("AUDIT_KMOD_NAME") ?: "dummy", 19);
    sprintf(module_path, "%s/%s.ko", getenv("AUDIT_KMOD_DIR") ?: ".", 
	    module_name);
    fprintf(stderr, "Module path: %s\n", module_path);
    errno = 0;
    fd = open(module_path, O_RDONLY);
    if (fd < 0) {
	fprintf(stderr, "Error: opening module path %s: %s\n", 
		module_path, strerror(errno));
	return -1;
    }
    rc = fstat(fd, &mstat);
    if (rc < 0) {
	rc = -1;
	fprintf(stderr, "Error: getting module file size: %s\n", 
		strerror(errno));
	goto exit;
    }
    region = mmap(NULL, mstat.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
    if (region == MAP_FAILED) {
	rc = -1;
	fprintf(stderr, "Error: mmap(): %s\n", strerror(errno));
	goto exit;
    }
    rc = syscall(__NR_init_module, region, mstat.st_size, "");
    if (rc < 0) {
	fprintf(stderr, "Error: loading module: %s\n", strerror(errno));
	if (errno != EEXIST)
	    goto exit_mem;
    }
    fprintf(stderr, "Loaded module: %s\n", module_path);

    if (!success) {
	rc = seteuid_test();
	if (rc < 0)
	    goto exit_mod;
	context_setexperror(context, EPERM);
    }

    rc = context_setidentifiers(context);
    if (rc < 0)
	goto exit_suid;

    context_setbegin(context);
    fprintf(stderr, "Attempting %s(%s, %x)\n", 
	    context->u.syscall.sysname, module_name, 0);
    errno = 0;
    exit = syscall(context->u.syscall.sysnum, module_name, 0);
    context_setend(context);
    context_setresult(context, exit, errno);

exit_suid:
    errno = 0;
    if (!success && (seteuid(0) < 0))
	fprintf(stderr, "Error: seteuid(): %s\n", strerror(errno));

exit_mod:
    errno = 0;
    if ((exit < 0) && (syscall(__NR_delete_module, module_name, 0) < 0))
	fprintf(stderr, "Error removing module: %s: %s\n", 
		module_name, strerror(errno));

exit_mem:
    munmap(region, mstat.st_size);

exit:
    close(fd);
    return rc;
}
