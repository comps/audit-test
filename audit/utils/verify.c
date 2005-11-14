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
 * ======================================================================= 
 */

#include "includes.h"

/*
 * Verify test result is as expected.
 */
ts_exit verify_result(struct audit_data *context, int success)
{
    ts_exit rc = TEST_EXPECTED;
    struct audit_data logdata;

    if ((success && !context->success) || (!success && context->success)) {
	rc = TEST_UNEXPECTED;
	fprintf(stderr, "Warning: test return code unexpected\n");
	goto exit;
    }

    rc = audit_parse_log(context, &logdata);
    if (rc) {
	rc = TEST_ERROR;
	fprintf(stderr, "Error: parsing audit log\n");
	goto exit;
    }

    if (!context_match(context, &logdata)) {
	rc = TEST_UNEXPECTED;
	fprintf(stderr, "Warning: could not find matching audit record\n");
    }

exit:
    return rc;
}

/*
 * Compare two audit_context structs containing the system context and
 * the corresponding data from the log.
 */
int context_match(struct audit_data *context, struct audit_data *logdata)
{
    return 0;
}
