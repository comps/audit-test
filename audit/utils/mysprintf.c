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
**  FILE   : mysprintf.c
**
**  PURPOSE: This function is intended to emulate the library sprintf()
**           function, however it mallocs memory and returns a pointer
**           to the string (rather than dumping the string into an
**           argument char*.  This function greatly cleans up the test
**           case array definitions.
**
**           NOTE: This function currently only supports the %s and
**                 %d tags (for strings and integers).  Also note
**                 that it is not currently possible for the user
**                 to actually print "%s" or "%d".  This is a minor
**                 bug that may be fixed eventually.
**
**
**  HISTORY:
**    07/03 originated by Dustin Kirkland (k1rkland@us.ibm.com
**    07/03 Modified by Michael A. Halcrow <mike@halcrow.us>
**    05/04 Updates to suppress compile warnings by Kimberly D. Simon <kdsimon@us.ibm.com>
**
**********************************************************************/

#include "utils.h"
#include <stdio.h>
#include <stdarg.h>
#include <string.h>


char* mysprintf(char* fmt, ...) {
  char* str;
  char* hold;
  va_list ap;
  char *p, *sval;
  int ival;
  //int i;     // not needed?
  int strlength = strlen(fmt);

  // Scroll through the format string stopping at %d and %s
  // When you encounter a %s, get the next argument from the list
  //  and add the length of the string to the total string length
  //  that will be allocated.
  // When you encounter a %d, get the next argument from the list,
  //  allocate an arbitrarily large buffer (1024 bytes), sprintf the
  //  integer argument to the buffer, and add the length of that string
  //  to the total string length to be allocated.
  va_start(ap, fmt);
  for (p=fmt; *p; p++) {
    if ((*p == '%') && (*(p+1) == 's')) {
      sval = va_arg(ap, char*);
      strlength += strlen(sval) - 2;
      p++;
      continue;
    }
    if ((*p == '%') && (*(p+1) == 'd')) {
      ival = va_arg(ap, int);
      sval = (char*)malloc(1024);
      sprintf(sval, "%d", ival);
      strlength += strlen(sval) - 2;
      free(sval);
      p++;
      continue;
    }
  }
  va_end(ap);

  // Malloc memory for string to be returned.
  // NOTE: the caller must free() this pointer to avoid a memory leak.
  // The str variable will move around, while hold will not.  So hold will 
  //  be returned at end of function.
  str = hold = (char*)malloc(strlength+1);
  memset(str, '\0', strlength+1);

  va_start(ap, fmt);
  for (p=fmt; *p; p++) {
    if (*p != '%') {
      // As long as no % found, just print character for character to string buffer
      strncpy(str, p, 1);
      str++;
      continue;
    }
    // Else, % was found, so determine if its a %s or %d or just a %
    switch (*++p) {
      case 's':
        // %s, so get argument and print each character to string buffer
        for (sval = va_arg(ap, char*); *sval; sval++) {
          strncpy(str, sval, 1);
          str++;
        }
        break;
      case 'd':
        // %d, so get argument, convert to string, and print to string buffer
        ival = va_arg(ap, int);
        sval = (char*)malloc(1024);
        sprintf(sval, "%d", ival);
        strncpy(str, sval, strlen(sval));
        str += strlen(sval);
        free(sval);
        break;
      default:
        // else, just print the character to the string buffer
        strncpy(str, p, 1);
        str++;
        break;
    }
  }
  va_end(ap);

  return hold;
}


