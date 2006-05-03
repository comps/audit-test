/**********************************************************************
**   Copyright (C) International Business Machines  Corp., 2003
**   Copyright (C) Hewlett-Packard Development Company, L.P., 2005
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
**  FILE   : shadow_test_setup.c
**
**  PURPOSE: This file copies a known file to /etc/default/useradd if the
**           value sent is a TRUE it will backup first.
**
**
**  HISTORY:
**    08/03 originated by Kylene J. Smith (kylene@us.ibm.com)
**    12/05 Adapted by Matt Anderson <mra@hp.com>
**
**********************************************************************/

#include "includes.h"
#include "trustedprograms.h"

int ShadowTestSetup( int backupBool ) {

    int rc = 0;

    if ( backupBool ) {
	backupFile("/etc/default/useradd");
    }

/*  we don't need to chdir do we?
    if(( rc = chdir( cwd ) == -1 )) {
	printf( "Error changing to working directory [%s]: errno = [%i]\n", cwd, errno );
	goto EXIT;
    }
*/
    
    if (( rc = system("cp config/useradd /etc/default/useradd")  == -1 )) {
	printf( "Error installing /etc/default/useradd: errno = [%i]\n", errno );
	goto EXIT;
    }
    
    
 EXIT:
    return rc;
}
