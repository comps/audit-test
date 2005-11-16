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

/* Verify operation result */
ts_exit verify_opresult(struct audit_data *context, int success)
{
    ts_exit rc = TEST_EXPECTED;

    if (success && !context->success) {
	rc = TEST_UNEXPECTED;
	fprintf(stderr, "Expected operation success, got operation failure\n");
	goto exit;
    }

    if (!success && context->success) {
	rc = TEST_UNEXPECTED;
	fprintf(stderr, "Expected operation failure, got operation success\n");
	goto exit;
    }

    if (context->experror && context->experror != context->error) {
	rc = TEST_UNEXPECTED;
	fprintf(stderr,
		"Expected operation error %d, got operation error %d\n",
		context->experror, context->error);
    }

exit:
    return rc;
}

/* Verify presence or absence of log record */
ts_exit verify_logresult(struct audit_data *context, int expfind)
{
    ts_exit rc = TEST_EXPECTED;

    return rc;
}
