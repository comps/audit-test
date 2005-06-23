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

#include "utils.h"
#include "globals.h"

/*
** Get data necessary for audit record verification
**
** BUGBUG:
** This funciton will call some LAUS service API
** to obtain audit_id.
*/
int getLAUSData(laus_data* dataPtr) {

  int rc = 0;
  uid_t ruid, euid, suid;         // fsuid;   not needed?
  gid_t rgid, egid, sgid;         // fsgid;   not needed?

  printf5("getLAUSData\n");

  /*
  ** BUGBUG: Don't know how to get:
  ** msg_seqnr
  ** msg_audit_id
  */

  /*
  ** Get PID
  */
  if (( dataPtr->msg_pid != NO_PID_CHECK ) && ( dataPtr->msg_pid != NO_FORK))
      dataPtr->msg_pid = getpid();

  /*
  ** Get login uid from global data
  */
  dataPtr->msg_login_uid = login_uid;

  /*
  ** Get uids/gids
  **
  ** BUGBUG: Don't know how to get fsuid/fsgid
  */
  if ((rc = getresuid(&ruid, &euid, &suid)) != 0) {
    goto EXIT;
  }
  dataPtr->msg_ruid = ruid;
  dataPtr->msg_suid = suid;

  if ((rc = getresgid(&rgid, &egid, &sgid)) != 0) {
    goto EXIT;
  }
  dataPtr->msg_rgid = rgid;
  dataPtr->msg_sgid = sgid;

 EXIT:
  return rc;
}
