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
 *  test_umask.c
 *
 *  PURPOSE:
 *  Verify audit of changes to file creation mask.
 *
 *  SYSCALLS:
 *  umask()
 *
 *  TESTCASE: successful
 *  Set umask to current umask value.
 *
 *  NOTES:
 *  umask() always succeeds.
 */

#include "includes.h"
#include "syscalls.h"

int test_umask(struct audit_data *context)
{
    int rc = 0;
    int mask;

    /* save the current mask, so the syscall op is also the cleanup */
    mask = umask(022);

    rc = context_setidentifiers(context);
    if (rc < 0)
        goto exit;

    context_setbegin(context);
    context->u.syscall.exit = umask(mask);
    context_setend(context);

exit:
    return rc;
}
