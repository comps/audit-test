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
 *	Tspi_Data_Seal01.c
 *
 * DESCRIPTION
 *	This test will verify that Tspi_Data_Seal
 *		returns TSS_SUCCESS.
 *
 * ALGORITHM
 *	Setup:
 *		Create Context
 *		Connect Context
 *		Create Key Object
 *		Create Enc Data
 *		Load Key By UUID
 *		Create PCR Composite
 *		Set PCR Value
 *
 *	Test:
 *		Call Data_Seal then if it does not succeed
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
 *      This test case is currently only implemented for v1.1
 *
 * HISTORY
 *      Megan Schneider, mschnei@us.ibm.com, 6/04.
 *      Kent Yoder, shpedoikal@gmail.com, 09/13/04
 *        Added SRK auth calls
 *
 * RESTRICTIONS
 *	None.
 */

#include <stdio.h>
#include "common.h"


int
main( int argc, char **argv )
{
	char		version;

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
	char		*function = "Tspi_Data_Seal-trans02";
	TSS_HCONTEXT	hContext;
	TSS_HKEY	hSRK;
	TSS_HKEY	hKey, hSigningKey, hWrappingKey;
	TSS_HPOLICY	hSrkPolicy;
	BYTE		*rgbDataToSeal = "This is a test.  1 2 3.";
	BYTE		rgbPcrValue[20];
	TSS_HENCDATA	hEncData;
	TSS_HPCRS	hPcrComposite;
	UINT32		BlobLength;
	UINT32		ulDataLength = strlen(rgbDataToSeal);
	TSS_UUID	uuid;
	TSS_RESULT	result;
	TSS_HTPM	hTPM;

	TSS_FLAG	initFlags = TSS_KEY_TYPE_SIGNING | TSS_KEY_SIZE_2048  |
				TSS_KEY_VOLATILE | TSS_KEY_NO_AUTHORIZATION |
				TSS_KEY_NOT_MIGRATABLE;

	print_begin_test( function );

	memset(rgbPcrValue, 0x5a, sizeof(rgbPcrValue));

	result = connect_load_all(&hContext, &hSRK, &hTPM);
	if (result != TSS_SUCCESS) {
		print_error("Tspi_Context_Create ", result);
		exit(result);
	}

	result = Testsuite_Transport_Init(hContext, hSRK, hTPM, TRUE, TRUE, &hWrappingKey,
					  &hSigningKey);
	if (result != TSS_SUCCESS) {
		print_error("Testsuite_Transport_Init", result);
		Tspi_Context_Close(hContext);
		exit(result);
	}

		// create hKey
	result = Tspi_Context_CreateObject( hContext,
						TSS_OBJECT_TYPE_RSAKEY,
						initFlags, &hKey );
	if ( result != TSS_SUCCESS )
	{
		print_error( "Tspi_Context_CreateObject (hKey)", result );
		Tspi_Context_FreeMemory( hContext, NULL );
		Tspi_Context_Close( hContext );
		exit( result );
	}

	result = Tspi_Context_CreateObject( hContext,
						TSS_OBJECT_TYPE_ENCDATA,
						TSS_ENCDATA_SEAL, &hEncData );
	if ( result != TSS_SUCCESS )
	{
		print_error( "Tspi_Context_CreateObject (hEncData)", result );
		Tspi_Context_FreeMemory( hContext, NULL );
		Tspi_Context_Close( hContext );
		exit( result );
	}

	result = Tspi_Context_CreateObject( hContext, TSS_OBJECT_TYPE_PCRS,
					0, &hPcrComposite );
	if ( result != TSS_SUCCESS )
	{
		print_error( "Tspi_Context_CreateObject (hPcrComposite)",
				result );
		Tspi_Context_FreeMemory( hContext, NULL );
		Tspi_Context_Close( hContext );
		exit( result );
	}

	result = Tspi_PcrComposite_SetPcrValue( hPcrComposite, 8, 20, rgbPcrValue );
	if ( result != TSS_SUCCESS )
	{
		print_error( "Tspi_PcrComposite_SetPcrValue", result );
		Tspi_Context_FreeMemory( hContext, NULL );
		Tspi_Context_Close( hContext );
		exit( result );
	}
	result = Tspi_PcrComposite_SetPcrValue( hPcrComposite, 9, 20, rgbPcrValue );
	if ( result != TSS_SUCCESS )
	{
		print_error( "Tspi_PcrComposite_SetPcrValue", result );
		Tspi_Context_FreeMemory( hContext, NULL );
		Tspi_Context_Close( hContext );
		exit( result );
	}

		// Data Seal
	result = Tspi_Data_Seal( hEncData, hSRK, ulDataLength, rgbDataToSeal, hPcrComposite );
	if ( result != TSS_SUCCESS )
	{
		print_error( "Tspi_Data_Seal", result );
		Tspi_Context_FreeMemory( hContext, NULL );
		Tspi_Context_Close( hContext );
		exit( result );
	}

	result = Testsuite_Transport_Final(hContext, hSigningKey);
	if ( result != TSS_SUCCESS )
	{
		if( !(checkNonAPI(result)) )
		{
			print_error( function, result );
		}
		else
		{
			print_error_nonapi( function, result );
		}
	}
	else
	{
		print_success( function, result );
	}

	print_end_test( function );
	Tspi_Context_FreeMemory( hContext, NULL );
	Tspi_Context_Close( hContext );
	exit(result);
}
