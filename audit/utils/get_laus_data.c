/**********************************************************************
**   Copyright (C) International Business Machines  Corp., 2003
**
**   This program is free software;  you can redistribute it and/or modify
**   it under the terms of the GNU General Public License as published by
**   the Free Software Foundation; either version 2 of the License, or
**   (at your option) any later version.
**
**   This program is distributed in the hope that it will be useful,
**   but WITHOUT ANY WARRANTY;  without even the implied warranty of
**   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See
**   the GNU General Public License for more details.
**
**   You should have received a copy of the GNU General Public License
**   along with this program;  if not, write to the Free Software
**   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
**
**
**
**  FILE   : get_laus_data.c
**
**  PURPOSE: This file defines a function that obtains information from
**           the Linux Auditing System to identify an audit record.
**           This identity information is needed to verify that the log
**           is accurate.
**
**
**  HISTORY:
**    06/03 originated by Dan Jones (danjones@us.ibm.com)
**    05/04 Updates to suppress compile warnings by Kimberly D. Simon <kdsimon@.us.ibm.com>
**
**********************************************************************/

#include "includes.h"

/*
** Get data necessary for audit record verification
**
** BUGBUG:
** This funciton will call some LAUS service API
** to obtain audit_id.
*/
int getLAUSData(struct audit_data *context)
{

    int rc = 0;
    uid_t ruid;		// XXX currently unused
    gid_t rgid;		// XXX currently unused
    uid_t euid, suid;	// fsuid;   not needed?
    gid_t egid, sgid;	// fsgid;   not needed?

    /*
     ** Get PID
     */
    if ((context->pid != NO_PID_CHECK) && (context->pid != NO_FORK))
	context->pid = getpid();

    /*
     ** Get uids/gids
     **
     ** BUGBUG: Don't know how to get fsuid/fsgid
     */
    if ((rc = getresuid(&ruid, &euid, &suid)) != 0) {
	goto EXIT;
    }
    context->suid = suid;

    if ((rc = getresgid(&rgid, &egid, &sgid)) != 0) {
	goto EXIT;
    }
    context->sgid = sgid;

  EXIT:
    return rc;
}
