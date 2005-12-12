/* =======================================================================
 *   (c) Copyright Hewlett-Packard Development Company, L.P., 2005 
 *   Written by Amy Griffis <amy.griffis@hp.com>
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
 * ======================================================================= 
 */

#include "includes.h"

char *audit_add_watch(const char *path)
{
    char *k, *key = NULL;
    char cmd[512] = { 0 };
    int count;

    if (!path)
	return NULL;

    key = strdup(path);
    for (k = key; *k != '\0'; k++)
	if (*k == '/')
	    *k = '_';

    count = snprintf(cmd, sizeof(cmd), "/sbin/auditctl -w %s -k %s",
		     path, key);
    if (count >= sizeof(cmd)) {
	fprintf(stderr, "Error: audit_add_watch(): cmd too long\n");
	goto exit_err;
    }

    fprintf(stderr, "%s\n", cmd);
    if (system(cmd) < 0) {
	fprintf(stderr, "Error: adding watch\n");
	goto exit_err;
    }

    return key;

exit_err:
    free(key);
    return NULL;
}

void audit_rem_watch(const char *path, const char *key)
{
    char cmd[512] = { 0 };
    int count;

    count = snprintf(cmd, sizeof(cmd), "/sbin/auditctl -W %s -k %s",
		     path, key);
    if (count >= sizeof(cmd)) {
	fprintf(stderr, "Error: audit_rem_watch(): cmd too long\n");
	return;
    }

    fprintf(stderr, "%s\n", cmd);
    if (system(cmd) < 0)
	fprintf(stderr, "Error: removing watch\n");
}
