/**********************************************************************
**   Copyright (C) International Business Machines  Corp., 2003
**   (c) Copyright Hewlett-Packard Development Company, L.P., 2005
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
**********************************************************************/

#ifndef _INCLUDES_H
#define _INCLUDES_H

#include <errno.h>        // Needed for all tests
#include <unistd.h>       // Needed for most tests
#include <asm/unistd.h>   // Needed for most tests
#include <sys/types.h>    // Needed for most tests
#include <sys/stat.h>     // Needed for file-related tests
#include <fcntl.h>	  // Needed for file related tests
#include <limits.h>	  // Needed for most tests
#include <sys/fsuid.h>    // Needed for file system check tests
#include <sys/socket.h>   // Needed for network tests
#include <netdb.h>        // Needed for network tests
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <asm/types.h>

#include "testsuite.h"

#endif
