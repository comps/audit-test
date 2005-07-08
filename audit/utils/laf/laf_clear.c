/* =======================================================================
 * Copyright (C) 2005 Hewlett-Packard Company
 * Written by Amy Griffis <amy.griffis@hp.com>
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

int laf_clear_logs()
{
    int rc = 0;
    int i, fd;
    int num_logs = 99;		// max according to manpage
    char *log_file = "/var/log/audit/audit.log";
    char *log_name = NULL;

    /* TODO: get log_file and num_logs from /etc/auditd.conf */

    if ((fd = open(log_file, O_WRONLY|O_TRUNC)) == -1) {
	rc = -1;
	goto exit_error;
    }
    if (close(fd) == -1) {
	rc = -1;
	goto exit_error;
    }

    /* add room for a 2-digit log file extension */
    if ((log_name = (char *)malloc(strlen(log_file) + 4)) == NULL) {
	rc = -1;
	goto exit_error;
    }

    for (i = 1; i < num_logs; i++) {

	if (sprintf(log_name, "%s.%d", log_file, i) < 0) {
	    rc = -1;
	    goto exit_error;
	}

	if ((fd = open(log_name, O_WRONLY|O_TRUNC)) == -1) {
	    rc = (errno == ENOENT ? 0 : -1);
	    goto exit_error;
	}
	if (close(fd) == -1) {
	    rc = -1;
	    goto exit_error;
	}
    }

exit_error:
    if (log_name) {
	free(log_name);
    }
    if (rc < 0) {
	printf1("ERROR: Unable to clear audit logs: %s\n", strerror(errno));
    }

    return rc;
}
