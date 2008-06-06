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
 *	Tspi_PcrComposite_GetPcrValue01.c
 *
 * DESCRIPTION
 *	This test will verify that Tspi_PcrComposite_GetPcrValue
 *		returns TSS_SUCCESS.
 *
 * ALGORITHM
 *	Setup:
 *		Create Context
 *		Connect Context
 *		Create PCR Composite
 *
 *	Test:
 *		Call PcrComposite_GetPcrValue then if it does not succeed
 *		Make sure that it returns the proper return codes
 *		Print results
 *
 *	Cleanup:
 *		Print errno log and/or timing stats if options given
 *
 * USAGE
 *      First parameter is --options
 *                         -v or --version
 *      Second parameter is the version of the test case to be run
 *      This test case is currently only implemented for v1.1
 *
 * HISTORY
 *      Megan Schneider, mschnei@us.ibm.com, 6/04.
 *      Kent Yoder, shpedoikal@gmail.com, 10/15/04
 *        Fixed object init flags
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

	version = parseArgs(argc, argv);
	if (version)
		main_v1_1();
	else
		print_wrongVersion();
}

int
main_v1_1( void )
{
	char		*function = "Tspi_PcrComposite_GetPcrValue01";
	TSS_HCONTEXT	hContext;
	TSS_HPCRS	hPcrComposite;
	BYTE		rgbPcrValueIn[20] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
	BYTE		*prgbPcrValueOut;
	UINT32		ulPcrValueLength;
	TSS_RESULT	result;
	BYTE		value[20] = {19,18,17,16,15,14,13,12,11,10,9,8,7,6,5,4,3,2,1,0};

	print_begin_test( function );

		// Create Context
	result = Tspi_Context_Create( &hContext );
	if ( result != TSS_SUCCESS )
	{
		print_error( "Tspi_Context_Create", result );
		print_error_exit( function, err_string(result) );
		exit( result );
	}

		// create object
	result = Tspi_Context_CreateObject( hContext, TSS_OBJECT_TYPE_PCRS,
					0, &hPcrComposite );
	if ( result != TSS_SUCCESS )
	{
		print_error( "Tspi_Context_CreateObject", result );
		print_error_exit( function, err_string(result) );
		Tspi_Context_FreeMemory( hContext, NULL );
		Tspi_Context_Close( hContext );
		exit( result );
	}

	result = Tspi_PcrComposite_SetPcrValue(hPcrComposite, 8,
			sizeof(value), value);
	if ( result != TSS_SUCCESS )
	{
		print_error( "Tspi_PcrComposite_SetPcrValue", result );
		print_error_exit( function, err_string(result) );
		Tspi_Context_FreeMemory( hContext, NULL );
		Tspi_Context_Close( hContext );
		exit( result );
	}

		// get pcr value
	result = Tspi_PcrComposite_GetPcrValue( hPcrComposite, 8,
						&ulPcrValueLength,
						&prgbPcrValueOut );
	if ( result != TSS_SUCCESS )
	{
		if( !(checkNonAPI(result)) )
		{
			print_error( function, result );
			print_end_test( function );
			Tspi_Context_FreeMemory( hContext, NULL );
			Tspi_Context_Close( hContext );
			exit(result);
		}
		else
		{
			print_error_nonapi( function, result );
			print_end_test( function );
			Tspi_Context_FreeMemory( hContext, NULL );
			Tspi_Context_Close( hContext );
			exit(result);
		}
	}
	else
	{
		result = Tspi_Context_FreeMemory(hContext, prgbPcrValueOut);
		if (result != TSS_SUCCESS) {
			print_error("Tspi_Context_FreeMemory ", result);
			print_error_exit(function, err_string(result));
			Tspi_Context_Close(hContext);
			exit(result);
		}
		print_success( function, result );
		print_end_test( function );
		Tspi_Context_FreeMemory( hContext, NULL );
		Tspi_Context_Close( hContext );
		exit( 0 );
	}
}
