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
   **  FILE       : test_rename.c
   **
   **  PURPOSE    : To test the rename library call auditing.
   **
   **  DESCRIPTION: The test_rename() function builds into the
   **  laus_test framework to verify that the Linux Audit System
   **  accurately logs both successful and erroneous execution of the
   **  "rename" system call.
   **
   **  In the successful case, this function:
   **   1) Generates an originating temporary file
   **   2) Generates a target temporary filename
   **   3) Makes the rename syscall 
   **
   **  The successful case renames the originating file to
   **  the target filename.  Since the originating filename
   **  is created immediately before renaming it to another unique,
   **  nonexistent name, we can expect rename() to successfully execute.
   **  
   **  The erroneous case invokes an error by trying to rename a file
   **  into the /root directory as a non-root user.
   **  According to the man page, an EACCES error is generated.
   **
   **  HISTORY    :
   **    06/03 Originated by Michael A. Halcrow <mike@halcrow.us>
   **    06/03 Furthered by Dustin Kirkland <k1rkland@us.ibm.com>
   **    03/04 Added exp_errno variable by D. Kent Soper <dksoper@us.ibm.com>
   **
   **********************************************************************/
   
#include "includes.h"
#include "syscalls.h"
#include <libgen.h>
   
int test_rename(laus_data *dataPtr) {

    int rc = 0;
    int exp_errno = EACCES;

    char* path = NULL;
    char* targetPath = NULL;

    // Set the syscall-specific data
    printf5("Setting laus_var_data.syscallData.code to %d\n", AUDIT_rename);
    dataPtr->laus_var_data.syscallData.code = AUDIT_rename;
     
    if ((rc = createTempFile(&path, S_IRWXU | S_IRWXG | S_IRWXO,
                              dataPtr->msg_euid, dataPtr->msg_egid)) == -1) {
        printf1("ERROR: Cannot create file %s\n", path);
        goto EXIT;
    }

    if (dataPtr->successCase) {
        // dynamically create target temp file name
        if ((rc = createTempFileName(&targetPath)) == -1) {
            printf1("ERROR: Cannot create file %s\n", targetPath);
            strcpy(targetPath, path);
            goto EXIT_CLEANUP;
        }
    } else {
        targetPath = strdup("/root/");
        realloc(targetPath, strlen(path));
        strcat(targetPath, basename(path));
    }
   
    // Set up audit argument buffer
    if ((rc = auditArg2(dataPtr, AUDIT_ARG_PATH, strlen(path), path, 
                        dataPtr->successCase ? AUDIT_ARG_PATH : AUDIT_ARG_STRING, 
                        strlen(targetPath), targetPath)) != 0) {
        printf1("Error setting up audit argument buffer\n");
        goto EXIT_CLEANUP;
    }
     
    // Fill in laus_data structure
    printf5("Calling getLAUSData\n");
    if ((rc = getLAUSData( dataPtr)) != 0) {
        printf1("Error returned from getLAUSData( dataPtr ): rc=%i\n", rc);
        goto EXIT_CLEANUP;
    }

    // Do pre-system call work
    if ((rc = preSysCall(dataPtr)) != 0) {
        printf1("ERROR: pre-syscall setup failed (%d)\n", rc);
        goto EXIT_CLEANUP;
    }

    // Execute system call
    dataPtr->laus_var_data.syscallData.result = 
        syscall(__NR_rename, path, targetPath);

    // Do post-system call work
    if ((rc = postSysCall(dataPtr, errno, -1, exp_errno)) != 0 ) {
        printf1("ERROR: post-syscall setup failed (%d)\n", rc);
        goto EXIT_CLEANUP;
    }
   
EXIT_CLEANUP:
    if (dataPtr->successCase) {
        // remove the target temporary file
        printf5("Removing file %s\n", targetPath);
        if (unlink(targetPath) == -1) {
            printf1("Error removing file %s: errno=%i\n", targetPath, errno);
            goto EXIT;
        }
    } else {
        printf5("Removing file %s\n", path);
        if( unlink(path) == -1) {
            printf1("Error removing file %s: errno=%i\n", path, errno);
            goto EXIT;
        }
    }
   
EXIT:
    if (targetPath) {
        free(targetPath);
    }
    if (path) {
        free(path);
    }
    printf5("Returning from test\n");
    return rc;
}
