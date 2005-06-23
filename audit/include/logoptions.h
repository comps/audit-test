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
**  FILE   : logoptions.h
**
**  PURPOSE: This file contains the type definition for the log
**  options for the EAL audit tests.
**
**  HISTORY:
**    10/03 Originated by Michael A. Halcrow <mike@halcrow.us>
**
**********************************************************************/

#ifndef _LAUSTEST_LOGOPTIONS_H
#define _LAUSTEST_LOGOPTIONS_H

// Array is used to loop through the various combinations of logSuccess
// and logFailure.
//
// The first value is for logSuccess the second valud is for logFailure.
//
// When selecting specific behavior, use the symbolic constants defined
// in types.h as index, i.e. logOptions[LOGOPTION_INDEX_ALL]
static log_options logOptions[] = {
    { TRUE, TRUE    } ,
    { TRUE,  FALSE  } ,
    { FALSE, TRUE   } ,
    { FALSE,  FALSE  } 
};

#endif
