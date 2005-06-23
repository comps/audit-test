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
**  FILE   : failsafe_array.h
**
**  PURPOSE: This file contains the array definition for all of the
**           failsafe tests.
**
**
**  HISTORY:
**    09/03 originated by Tom Lendacky (toml@us.ibm.com)
**
**********************************************************************/

#ifndef _FAILSAFE_ARRAY_H
#define _FAILSAFE_ARRAY_H

#include "failsafe.h"


// Based on tests being run as root
failsafe_data failsafe_tests[] = {
	// Log rollover test
	{ &test_rollover, "log rollover", NULL },
	// Log rollover failure test (notify program failure)
	{ &test_rollover_failure, "log rollover failure", NULL },
};

#endif
