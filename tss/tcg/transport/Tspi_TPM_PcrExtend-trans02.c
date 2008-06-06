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
 *	Tspi_TPM_PcrExtend01.c
 *
 * DESCRIPTION
 *	This test will verify Tspi_TPM_PcrExtend.
 *	The purpose of this test case is to get TSS_SUCCESS to be returned.
 *		This is accomplished by following the algorithm below
 *
 * ALGORITHM
 *	Setup:
 *		Create Context
 *		Connect Context
 *		GetTPMObject
 *
 *	Test:   Call PcrExtend. If this is unsuccessful check for 
 * 		type of error, and make sure it returns the proper return code
 * 
 *	Cleanup:
 *		Close Context
 *		Print error/success message
 *
 * USAGE:	First parameter is --options
 *			-v or --version
 *		Second Parameter is the version of the test case to be run.
 *		This test case is currently only implemented for 1.1
 *
 * HISTORY
 *	Author:	Kathy Robertson
 *	Date:	June 2004
 *	Email:	klrobert@us.ibm.com
 *
 * RESTRICTIONS
 *	None.
 */

#include "common.h"


int main(int argc, char **argv)
{
	char version;

	version = parseArgs( argc, argv );
	if (version == TESTSUITE_TEST_TSS_1_2)
		main_v1_2(version);
	else if (version == TESTSUITE_TEST_TSS_1_1)
		print_NA();
	else
		print_wrongVersion();
}

int
main_v1_2(char version)
{
	char		*nameOfFunction = "Tspi_TPM_PcrExtend-trans02";
	TSS_HCONTEXT	hContext;
	TSS_HTPM	hTPM;
	BYTE		pcrValue;
	UINT32		ulNewPcrValueLength;
	BYTE*		NewPcrValue;
	TSS_RESULT	result;
	TSS_HKEY	hSRK, hSigningKey, hWrappingKey;

	TSS_PCR_EVENT event;
	memset(&event, 0, sizeof(TSS_PCR_EVENT));
	event.ulPcrIndex = 9;

	print_begin_test(nameOfFunction);


	result = connect_load_all(&hContext, &hSRK, &hTPM);
	if ( result != TSS_SUCCESS )
	{
		print_error( "connect_load_all", (result) );
		Tspi_Context_FreeMemory( hContext, NULL );
		Tspi_Context_Close( hContext );
		exit( result );
	}

	result = Testsuite_Transport_Init(hContext, hSRK, hTPM, FALSE, TRUE, &hWrappingKey,
					  &hSigningKey);
	if (result != TSS_SUCCESS) {
		print_error("Testsuite_Transport_Init", result);
		Tspi_Context_Close(hContext);
		exit(result);
	}
		//Call PcrExtend
	result = Tspi_TPM_PcrExtend(hTPM, 9, 20,
			&pcrValue, &event, &ulNewPcrValueLength, &NewPcrValue);
	if (result != TSS_SUCCESS) {
		print_error("Tspi_TPM_PcrExtend", result);
		Tspi_Context_Close(hContext);
		exit(result);
	}

	result = Testsuite_Transport_Final(hContext, hSigningKey);
	if (result != TSS_SUCCESS) {
		if(!checkNonAPI(result)){
			print_error(nameOfFunction, result);
			print_end_test(nameOfFunction);
			Tspi_Context_Close(hContext);
			exit(result);
		}
		else{
			print_error_nonapi(nameOfFunction, result);
			print_end_test(nameOfFunction);
			Tspi_Context_Close(hContext);
			exit(result);
		}
	}
	else{
		result = Tspi_Context_FreeMemory(hContext, NewPcrValue);
		if (result != TSS_SUCCESS) {
			print_error("Tspi_Context_FreeMemory ", result);
			Tspi_Context_Close(hContext);
			exit(result);
		}
		print_success(nameOfFunction, result);
		print_end_test(nameOfFunction);
		Tspi_Context_Close(hContext);
		exit(0);
	}
}
