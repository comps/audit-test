/* =======================================================================
 * Copyright (C) 2005 Hewlett-Packard Company
 * 
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License.
 * 
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 * 
 *   You should have received a copy of the GNU General Public License
 *   with this package; if not, write to the Free Software Foundation,
 *   Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 *
 *   Utility functions for initializing temporary files and
 *   directories.
 * 
 * ======================================================================= 
 */

#include "includes.h"
#include <libgen.h>

static int common_setperms(char *name, mode_t mode, uid_t uid, gid_t gid, 
			   int file)
{
    if ((mode != -1) &&
	(chmod(name, mode) < 0)) {
	fprintf(stderr, "Error: initializing temp%s: chmod(%d): %s\n",
		file ? "file" : "dir", mode, strerror(errno));
	return -1;
    }

    if (chown(name, uid, gid) < 0) {
	fprintf(stderr, "Error: initializing temp%s: chown(%d, %d): %s\n",
		file ? "file" : "dir", uid, gid, strerror(errno));
	return -1;
    }

    return 0;
}

char *init_tempdir(mode_t mode, uid_t uid, gid_t gid)
{
    char *dname;

    errno = 0;
    dname = (char *)malloc(strlen(TEST_TMP_TEMPLATE) + 1);
    if (!dname) {
	fprintf(stderr, "Error: initializing tempdir: malloc(): %s\n",
		strerror(errno));
	return NULL;
    }

    strcpy(dname, TEST_TMP_TEMPLATE);
    if (mkdtemp(dname) == NULL) {
	fprintf(stderr, "Error: initializing tempdir: mkdtemp(): %s\n",
		strerror(errno));
	free(dname);
	return NULL;
    }

    fprintf(stderr, "Created tempdir: %s\n", dname);

    if (common_setperms(dname, mode, uid, gid, 0) < 0)
	goto exit_err;

    if (mode == -1)
	fprintf(stderr, "Permissions: %d:%d %s\n", uid, gid, dname);
    else
	fprintf(stderr, "Permissions: [%o] %d:%d %s\n", mode, uid, gid, dname);

    return dname;

exit_err:
    destroy_tempdir(dname);
    return NULL;
}

char *init_tempfile(mode_t mode, uid_t uid, gid_t gid, char *name)
{
    int fd = 0;
    char *fname;
    char data[512];
    int count;

    errno = 0;
    count = snprintf(data, sizeof(data), 
		     "This tempfile created for testing %s audits.\n", name);
    if (count >= sizeof(data)) {
	fprintf(stderr, "Error: initializing tempfile: [%s] too long\n", name);
	return NULL;
    }

    fname = (char *)malloc(strlen(TEST_TMP_TEMPLATE) + 1);
    if (!fname) {
	fprintf(stderr, "Error: initializing tempfile: malloc(): %s\n", 
		strerror(errno));
	return NULL;
    }

    strcpy(fname, TEST_TMP_TEMPLATE);
    fd = mkstemp(fname);
    if (fd < 0) {
	fprintf(stderr, "Error: initializing tempfile: mkstemp(): %s\n", 
		strerror(errno));
	free(fname);
	return NULL;
    }

    fprintf(stderr, "Created tempfile: %s\n", fname);

    if (write(fd, data, count) < 0) {
	fprintf(stderr, "Error: initializing tempfile: write(): %s\n",
		strerror(errno));
	goto exit_err;
    }

    if (close(fd) < 0) {
	fprintf(stderr, "Error: initializing tempfile: close(): %s\n",
		strerror(errno));
	goto exit_err;
    }

    if (common_setperms(fname, mode, uid, gid, 1) < 0)
	goto exit_err;

    fprintf(stderr, "Permissions: [%o] %d:%d %s\n", mode, uid, gid, fname);

    return fname;

exit_err:
    destroy_tempfile(fname);
    return NULL;
}

char *init_tempswap(mode_t mode, uid_t uid, gid_t gid, size_t size)
{
    char *data, *path;
    int fd;
    char cmd[512] = { 0 };
    int count;

    errno = 0;
    data = malloc(size);
    if (!data) {
	fprintf(stderr, "Error: initializing swapfile: malloc(): %s\n",
		strerror(errno));
	return NULL;
    }
    memset(data, 0, size);

    path = init_tempfile(mode, uid, gid, "");
    if (!path) {
	fprintf(stderr, "Error: initializing temporary swapfile\n");
	goto exit;
    }

    fd = open(path, O_WRONLY|O_TRUNC);
    if (fd < 0) {
	fprintf(stderr, "Error: initializing swapfile: open(): %s\n",
		strerror(errno));
	goto exit_err;
    }

    if (write(fd, data, size) < size) {
	fprintf(stderr, "Error: initializing swapfile: write(): %s\n",
		strerror(errno));
	goto exit_err;
    }

    if (close(fd) < 0) {
	fprintf(stderr, "Error: initializing swapfile: close(): %s\n",
		strerror(errno));
	goto exit_err;
    }

    count = snprintf(cmd, sizeof(cmd), "/sbin/mkswap %s", path);
    if (count >= sizeof(cmd)) {
	fprintf(stderr, "Error: initializing swapfile: cmd too long\n");
	goto exit_err;
    }
    if (system(cmd) < 0) {
	fprintf(stderr, "Error: initializing swapfile: /sbin/mkswap\n");
	goto exit_err;
    }
	
exit:
    free(data);
    return path;

exit_err:
    free(data);
    destroy_tempfile(path);
    return NULL;
}

char *init_tempsym(char *target, uid_t uid, gid_t gid)
{
    char *spath, *tmp;
    char *sname = "/sym";

    tmp = init_tempdir(-1, uid, gid);
    if (!tmp) {
	fprintf(stderr, "Error: initializing symlink directory\n");
	spath = NULL;
	goto exit;
    }

    spath = realloc(tmp, strlen(tmp) + strlen(sname) + 1);
    if (!spath) {
	fprintf(stderr, "Error: initializing symlink: realloc(): %s\n",
		strerror(errno));
	destroy_tempdir(tmp);
	spath = NULL;
	goto exit;
    }
    if (strcat(spath, sname) == NULL) {
	fprintf(stderr, "Error: initializing symlink: strcat(): %s\n",
		strerror(errno));
	destroy_tempdir(spath);
	spath = NULL;
	goto exit;
    }
    if (symlink(target, spath) < 0) {
	fprintf(stderr, "Error: initializing symlink: symlink(): %s\n",
		strerror(errno));
	destroy_tempdir(spath);
	spath = NULL;
	goto exit;
    }

    fprintf(stderr, "Created symlink: %s -> %s\n", spath, target);

    if (lchown(spath, uid, gid) < 0) {
	fprintf(stderr, "Error: initializing sysmlink: chown(): %s\n", 
		strerror(errno));
	destroy_tempsym(spath);
	spath = NULL;
    }

    fprintf(stderr, "Permissions: %d:%d %s\n", uid, gid, spath);

exit:
    return spath;
}

void destroy_tempdir(char *name)
{
    errno = 0;
    if (rmdir(name) < 0)
	fprintf(stderr, "Error: removing tempdir: rmdir(%s): %s\n",
		name, strerror(errno));
    else
	fprintf(stderr, "Removed tempdir: %s\n", name);
    free(name);
}

void destroy_tempfile(char *name)
{
    errno = 0;
    if (unlink(name) < 0)
	fprintf(stderr, "Error: removing tempfile: unlink(%s): %s\n",
		name, strerror(errno));
    else
	fprintf(stderr, "Removed tempfile: %s\n", name);
    free(name);
}

void destroy_tempsym(char *name)
{
    errno = 0;
    if (unlink(name) < 0) {
	fprintf(stderr, "Error: removing symlink: unlink(%s): %s\n",
		name, strerror(errno));
	return;
    }
    fprintf(stderr, "Removed symlink: %s\n", name);

    if (rmdir(dirname(name)) < 0)
	fprintf(stderr, "Error: removing symlink: rmdir(%s): %s\n",
		name, strerror(errno));
    else
	fprintf(stderr, "Removed tempdir: %s\n", name);

    free(name);
}
