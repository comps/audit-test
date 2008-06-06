/*
 *
 *   Copyright (C) International Business Machines  Corp., 2004-2006
 *
 *   This program is free software;  you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY;  without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See
 *   the GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program;  if not, write to the Free Software
 *   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 */

/*
 * NAME
 *	Tspi_Hash_GetHashValue02.c
 *
 * DESCRIPTION
 *	This test will return TSS_E_INVALID_HANDLE because
 *		whHash=-1 is passed as the first parameter to
 *		Hash_SetHashValue, rather than hHash.
 *
 * ALGORITHM
 *	Setup:
 *		Create Context
 *		Connect Context
 *
 *	Test:
 *		Call Hash_GetHashValue then if it does not succeed
 *		Make sure that it returns the proper return codes
 *		Print results
 *
 *	Cleanup:
 *		Free memory related to hContext
 *		Close context
 *
 * USAGE
 *      First parameter is --options
 *                         -v or --version
 *      Second parameter is the version of the test case to be run
 *      This test case is currently only implemented for v1.1 and 1.2
 *
 * HISTORY
 *      Megan Schneider, mschnei@us.ibm.com, 6/04.
 *	EJR, ejratl@gmail.com, 8/10/2006, 1.2
 *
 * RESTRICTIONS
 *	None.
 */

#include <stdio.h>
#include "common.h"


int main(int argc, char **argv)
{
	char version;

	version = parseArgs(argc, argv);
	if (version)
		main_v1_1();
	else
		print_wrongVersion();
}

int main_v1_1(void)
{
	char *function = "Tspi_Hash_GetHashValue02";
	TSS_HCONTEXT hContext;
	TSS_HHASH whHash = -1;
	UINT32 ulHashValueLength, exitCode = 0;
	BYTE *rgbHashValue;
	TSS_RESULT result;

	print_begin_test(function);

	// Create Context
	result = Tspi_Context_Create(&hContext);
	if (result != TSS_SUCCESS) {
		print_error("Tspi_Context_Create", result);
		exit(result);
	}
	// Connect to Context
	result = Tspi_Context_Connect(hContext, get_server(GLOBALSERVER));
	if (result != TSS_SUCCESS) {
		print_error("Tspi_Context_Connect", result);
		Tspi_Context_FreeMemory(hContext, NULL);
		Tspi_Context_Close(hContext);
		exit(result);
	}

	result = Tspi_Hash_GetHashValue(whHash, &ulHashValueLength,
					&rgbHashValue);
	if (TSS_ERROR_CODE(result) != TSS_E_INVALID_HANDLE) {
		if (!(checkNonAPI(result))) {
			print_error(function, result);
		} else {
			print_error_nonapi(function, result);
		}
		exitCode = result;
	} else {
		print_success(function, result);
	}

	print_end_test(function);
	Tspi_Context_FreeMemory(hContext, NULL);
	Tspi_Context_Close(hContext);
	exit(exitCode);
}
