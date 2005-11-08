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
**  FILE   : rc_return.c
**
**  PURPOSE: This file is a generic routine used to return a return
**           code that is supplied as an input argument to the routine.
**
**  HISTORY:
**    08/03 originated by Tom Lendacky (toml@us.ibm.com)
**    05/04 Updates to suppress compile warnings by Kimberly Simon <kdsimon@us.ibm.com>
**
**********************************************************************/
#include <stdlib.h>

int main(int argc, char **argv)
{

    int rc;


    if (argc < 2)
	return -1;

    rc = atoi(argv[1]);

    return rc;
}
