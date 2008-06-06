/*
 *
 *   Copyright (C) International Business Machines  Corp., 2004, 2005
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
 *	Tspi_Context_Close02.c
 *
 * DESCRIPTION
 *	This test will verify Tspi_Context_Close
 *	This is a test case intended to make Context_Close
 *		return TSS_E_INVALID_HANDLE. This is
 *		accomplished by passing in an unallocated context handle.
 * ALGORITHM
 *	Setup:
 *		Create Context
 *
 *	Test:	Call Close Context
 *		Make sure that it returns the proper return codes
 *
 *	Cleanup:
 *		Print error/success message
 *
 * HISTORY
 *	Author:	Kathy Robertson
 *	Date:	June 2004
 *	Email:	klrobert@us.ibm.com
 *	Kent Yoder, shpedoikal@gmail.com, 1/05
 *
 * RESTRICTIONS
 *	None.
 */

#include <stdlib.h>

#include "common.h"



int main(int argc, char **argv)
{
	char		version;

	version = parseArgs( argc, argv );
	if (version)
		main_v1_1();
	else
		print_wrongVersion();
}

int
main_v1_1(void){

	char		*nameOfFunction = "Tspi_Context_Close02";
	TSS_HCONTEXT	hContext = 0xffffffff;
	TSS_RESULT	result;

	print_begin_test(nameOfFunction);

		//Close Context
	result = Tspi_Context_Close(hContext);
	if (TSS_ERROR_CODE(result) != TSS_E_INVALID_HANDLE) {
		if(!checkNonAPI(result)){
			print_error(nameOfFunction, result);
			print_end_test(nameOfFunction);
			exit(result);
		}
		else{
			print_error_nonapi(nameOfFunction, result);
			print_end_test(nameOfFunction);
			exit(result);
		}
	}
	else{
		print_success(nameOfFunction, result);
		print_end_test(nameOfFunction);
		exit(0);
	}
}
