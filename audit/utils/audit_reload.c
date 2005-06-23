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
**  FILE   : audit_reload.c
**
**  PURPOSE: This file defines a function that will signal the auditing
**           system to reload it's configuration.
**
**
**  HISTORY:
**    08/03 originated by Tom Lendacky (toml@us.ibm.com)
**
**********************************************************************/

#include "utils.h"

#include "laus.h"

/*
** Reload the auditing configuration
**
*/
int reloadAudit() {

	int rc = 0;


	printf4("reloadAudit\n");
	if ((rc = system("/sbin/auditd -r")) == -1) {
		printf1("ERROR: Unable to reload audit configuration errno=%d\n", errno);
	}

	return rc;
}
