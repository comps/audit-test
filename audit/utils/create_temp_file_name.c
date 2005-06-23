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
**  FILE   : create_temp_file_name.c
**
**  PURPOSE: This file defines a utility function that creates a unique
**           filename.  This function uses mkstemp() to create a file,
**           and then immediately deletes it, thus momentarily freeing
**           the filename.  Note that this is not a perfect solution.
**           Its possible, though hopefully unlikely, that another file
**           might be created with the same name in the time between
**           which the calling function requests a unique filename
**           and the time at which the calling function creates the file.
**           This is basically the same problem as using tempnam(). 
**           Risk is mitigated with a distinct prefix on the file.  As
**           long as the user understands this small probability, the
**           risk is likely acceptable.
**
**           Some test cases must operate on a unique filename.  It is 
**           this function that generates such filenames, with the 
**           caveats discussed above.
**
**
**  HISTORY:
**    06/03 originated by Dustin Kirkland (k1rkland@us.ibm.com)
**
**********************************************************************/

#include "utils.h"
#include "tempname.h"
#include <stdlib.h>

/*
** Create a test file
*/
int createTempFileName(char** fname) {

  int rc = 0;
  int fd = 0;

  *fname = (char *) malloc(strlen(tempname) + 1);
  strcpy(*fname, tempname);  

  printf5("createTempFileName: %s\n", *fname);

  if ((fd = mkstemp(*fname)) == -1) {
    printf1("ERROR: Unable to create %s: errno=%i\n", *fname, errno);
    rc = fd;
    goto EXIT;
  }
  printf5("temp file name: %s\n", *fname);
  if ((rc = close(fd)) == -1) {
    printf1("ERROR: Unable to close file %s: errno=%i\n", *fname, errno);
    goto EXIT;
  }
  if ((rc = unlink(*fname)) == -1) {
    printf1("ERROR: Unable to remove file %s: errno=%i\n", *fname, errno);
    goto EXIT;
  }

EXIT:
  return rc;

}

