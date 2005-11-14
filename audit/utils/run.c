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
**  FILE   : run.c
**
**  PURPOSE: This file defines a utility functions that forks
**           and execs a program returning the pid.
**           The input parameters contains the program name
**           followed by any command line arguments.
**
**  HISTORY:
**    08/03 originated by Daniel H. Jones (danjones@us.ibm.com)
**    05/04 Updates to suppress compile warnings by Kimberly D. Simon <kdsimon@us.ibm.com>
**
**********************************************************************/

#include "includes.h"
#include <sys/wait.h>

/*
** exec a program
**
** This function returns the pid of the exec'd process
*/
int run(char *string)
{

    int pid = 0;
    int index;
    int x;
    char *argv[64];
    char *envp[1] = { NULL };
    char *command = NULL;
    char *tmp;

    fprintf(stderr, "Executing: [%s]\n", string);

    // Copy command to non-static buffer
    command = (char *)malloc(strlen(string) + 1);
    strncpy(command, string, strlen(string) + 1);

    // Contruct arg vector
    index = 0;
    argv[index] = NULL;
    if ((tmp = strtok(command, " ")) != NULL) {
	argv[index] = (char *)malloc(strlen(tmp) + 1);
	strcpy(argv[index], tmp);
	index++;
	argv[index] = NULL;
	while ((tmp = strtok(NULL, " ")) != NULL) {
	    argv[index] = (char *)malloc(strlen(tmp) + 1);
	    strcpy(argv[index], tmp);
	    index++;
	    argv[index] = NULL;
	    if (index > 63) {
		fprintf(stderr, "Too many arguments\n");
		break;
	    }
	}
    }
    // Debug info
    index = 0;
    while (argv[index] != NULL) {
	fprintf(stderr, "arg %i = %s\n", index, argv[index]);
	index++;
    }

    // Run it ...
    if ((pid = fork()) == 0) {
	// We are in the child
	if (execve(argv[0], argv, envp) == -1) {
	    fprintf(stderr, "execve() error: errno=%i\n", errno);
	    exit(-1);
	}
    } else {
	// We are in the parent
	waitpid(pid, NULL, 0);
    }

// EXIT:         // not needed?

    for (x = 0; x < index; x++) {
	free(argv[x]);
    }

    return pid;
}
