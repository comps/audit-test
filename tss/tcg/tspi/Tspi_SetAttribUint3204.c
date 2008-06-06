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
 *	Tspi_SetAttribUint3204.c
 *
 * DESCRIPTION
 *	This test will return TSS_E_BAD_PARAMETER, because
 *		hParentKey is created with TSS_OBJECT_TYPE_HASH
 *		and TSS_HASH_SHA1, rather than TSS_OBJECT_TYPE_RSAKEY
 *		and initFlags, before being passed to SetAttribUint32.
 *
 * ALGORITHM
 *	Setup:
 *		Create Context
 *		Connect Context
 *		Create Object
 *
 *	Test:
 *		Call Tspi_SetAttribUint32 twice then if it does not succeed
 *		Make sure that it returns the proper return codes
 *		Print results
 *
 *	Cleanup:
 *		Free memory relating to hContext
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
 *
 * RESTRICTIONS
 *	None.
 */

#include <stdio.h>
#include "common.h"

int
main( int argc, char **argv )
{
	char version;

	version = parseArgs( argc, argv );
	if (version)
		main_v1_1();
	else
		print_wrongVersion();
}

int
main_v1_1( void )
{
	char			*function = "Tspi_SetAttribUint3204";
	TSS_HKEY		hParentKey;
	TSS_HCONTEXT		hContext;
	TSS_RESULT		result;
	UINT32			exitCode;

	print_begin_test( function );

		// Create Context
	result = Tspi_Context_Create( &hContext );
	if ( result != TSS_SUCCESS )
	{
		print_error( "Tspi_Context_Create", result );
		exit( result );
	}

		// Connect to Context
	result = Tspi_Context_Connect( hContext, get_server(GLOBALSERVER) );
	if ( result != TSS_SUCCESS )
	{
		print_error( "Tspi_Context_Connect", result );
		Tspi_Context_FreeMemory( hContext, NULL );
		Tspi_Context_Close( hContext );
		exit( result );
	}

		// Create and initialize empty object
	result = Tspi_Context_CreateObject( hContext,
						TSS_OBJECT_TYPE_HASH,
						TSS_HASH_SHA1, &hParentKey );
	if ( result != TSS_SUCCESS )
	{
		print_error( "Tspi_Context_CreateObject (parent key)", result );
		Tspi_Context_FreeMemory( hContext, NULL );
		Tspi_Context_Close( hContext );
		exit( result );
	}

		// Set uint, no encryption, key enc scheme
	result = Tspi_SetAttribUint32( hParentKey,
					TSS_TSPATTRIB_KEY_INFO,
					TSS_TSPATTRIB_KEYINFO_ENCSCHEME,
					TSS_ES_NONE );
	if ( TSS_ERROR_CODE(result) != TSS_E_BAD_PARAMETER )
	{
		if( !(checkNonAPI(result)) )
		{
			print_error( function, result );
			exitCode = 1;
		}
		else
		{
			print_error_nonapi( function, result );
			exitCode = 1;
		}
	}
	else
	{
		print_success( function, result );
		exitCode = 0;
	}

		// Set uint, key sig scheme
	result = Tspi_SetAttribUint32( hParentKey,
					TSS_TSPATTRIB_KEY_INFO,
					TSS_TSPATTRIB_KEYINFO_SIGSCHEME,
					TSS_SS_NONE );
	if ( TSS_ERROR_CODE(result) != TSS_E_BAD_PARAMETER )
	{
		if( !(checkNonAPI(result)) )
		{
			print_error( function, result );
			exitCode = 1;
		}
		else
		{
			print_error_nonapi( function, result );
			exitCode = 1;
		}
	}
	else
	{
		print_success( function, result );
	}

	print_end_test( function );
	Tspi_Context_FreeMemory( hContext, NULL );
	Tspi_Context_Close( hContext );
	exit( exitCode );
}
