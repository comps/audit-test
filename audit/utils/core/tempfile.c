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

    return dname;

exit_err:
    destroy_tempdir(dname);
    return NULL;
}

char *init_tempfile(mode_t mode, uid_t uid, gid_t gid)
{
    int fd = 0;
    char *fname;
    char *writedata = "This tempfile created for audit testing.\n";

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

    if (write(fd, writedata, strlen(writedata)) < 0) {
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

    return fname;

exit_err:
    destroy_tempfile(fname);
    return NULL;
}

char *init_tempsym(char *target, uid_t uid, gid_t gid)
{
    char *spath;
    char *sname = "/sym";

    spath = init_tempdir(-1, uid, gid);
    if (!spath) {
	fprintf(stderr, "Error: initializing symlink directory\n");
	spath = NULL;
	goto exit;
    }

    errno = 0;
    if (realloc(spath, strlen(spath) + sizeof(sname)) == NULL) {
	fprintf(stderr, "Error: initializing symlink: realloc(): %s\n",
		strerror(errno));
	destroy_tempdir(spath);
	spath = NULL;
	goto exit;
    }

    errno = 0;
    if (strcat(spath, sname) == NULL) {
	fprintf(stderr, "Error: initializing symlink: strcat(): %s\n",
		strerror(errno));
	destroy_tempdir(spath);
	spath = NULL;
	goto exit;
    }

    errno = 0;
    if (symlink(target, spath) < 0) {
	fprintf(stderr, "Error: initializing symlink: symlink(): %s\n",
		strerror(errno));
	destroy_tempdir(spath);
	spath = NULL;
	goto exit;
    }

    fprintf(stderr, "Created symlink: %s\n", spath);

    if (lchown(spath, uid, gid) < 0) {
	fprintf(stderr, "Error: initializing sysmlink: chown(): %s\n", 
		strerror(errno));
	destroy_tempsym(spath);
	spath = NULL;
    }

exit:
    return spath;
}

void destroy_tempdir(char *name)
{
    if (rmdir(name) < 0)
	fprintf(stderr, "Error: removing tempdir: rmdir(%s): %s\n",
		name, strerror(errno));
    else
	fprintf(stderr, "Removed tempdir: %s\n", name);
    free(name);
}

void destroy_tempfile(char *name)
{
    if (unlink(name) < 0)
	fprintf(stderr, "Error: removing tempfile: unlink(%s): %s\n",
		name, strerror(errno));
    else
	fprintf(stderr, "Removed tempfile: %s\n", name);
    free(name);
}

void destroy_tempsym(char *name)
{
    char *save_name;

    if (unlink(name) < 0) {
	fprintf(stderr, "Error: removing symlink: unlink(%s): %s\n",
		name, strerror(errno));
	return;
    }

    save_name = strdup(name);
    if (rmdir(dirname(name)) < 0)
	fprintf(stderr, "Error: removing symlink: rmdir(%s): %s\n",
		name, strerror(errno));
    else
	fprintf(stderr, "Removed symlink: %s\n", save_name);

    free(name);
    free(save_name);
}
