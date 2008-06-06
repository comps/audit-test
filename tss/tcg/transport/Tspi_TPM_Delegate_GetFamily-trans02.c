/*
 *
 *   Copyright (C) International Business Machines  Corp., 2007
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
 *	Tspi_TPM_Delegate_GetFamily01.c
 *
 * DESCRIPTION
 *	This test will verify that Tspi_TPM_Delegate_GetFamily
 *		returns TSS_SUCCESS.
 *
 * ALGORITHM
 *	Setup:
 *		Create Context
 *		Connect Context
 *		Get TPM Handle
 *              Create a Family
 *
 *	Test:
 *		Call TPM_Delegate_GetFamily.
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
 *      This test case is currently only implemented for v1.2
 *
 * HISTORY
 *      Tom Lendacky, toml@us.ibm.com, 7/07.
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
	if (version >= TESTSUITE_TEST_TSS_1_2)
		main_v1_2(version);
	else
		print_NA();
}

int
main_v1_2( char version )
{
	char *			function = "Tspi_TPM_Delegate_GetFamily-trans02";
	TSS_HCONTEXT		hContext;
	TSS_HKEY		hSRK, hSigningKey, hWrappingKey;
	TSS_HTPM		hTPM;
	TSS_HPOLICY		hTPMPolicy;
	TSS_HDELFAMILY		hFamily = NULL_HDELFAMILY;
	UINT32                  familyID, returnedID;
	TSS_RESULT		result;

	print_begin_test( function );

	result = connect_load_all(&hContext, &hSRK, &hTPM);
	if ( result != TSS_SUCCESS )
	{
		print_error( "connect_load_all", (result) );
		goto done;
	}

	result = Testsuite_Transport_Init(hContext, hSRK, hTPM, TRUE, TRUE, &hWrappingKey,
					  &hSigningKey);
	if (result != TSS_SUCCESS) {
		print_error("Testsuite_Transport_Init", result);
		Tspi_Context_Close(hContext);
		exit(result);
	}

	result = Tspi_GetPolicyObject( hTPM, TSS_POLICY_USAGE, &hTPMPolicy );
	if ( result != TSS_SUCCESS )
	{
		print_error( "Tspi_GetPolicyObject", result );
		goto done;
	}

	result = Tspi_Policy_SetSecret( hTPMPolicy, TESTSUITE_OWNER_SECRET_MODE,
					TESTSUITE_OWNER_SECRET_LEN, TESTSUITE_OWNER_SECRET );
	if ( result != TSS_SUCCESS )
	{
		print_error( "Tspi_Policy_SetSecret", result );
		goto done;
	}

	result = Tspi_TPM_Delegate_AddFamily(hTPM, 'a', &hFamily);
	if ( result != TSS_SUCCESS )
	{
		print_error( "Tspi_TPM_Delegate_AddFamily", (result) );
		goto done;
	}

	result = Tspi_GetAttribUint32(hFamily, TSS_TSPATTRIB_DELFAMILY_INFO,
			TSS_TSPATTRIB_DELFAMILYINFO_FAMILYID, &familyID);
	if ( result != TSS_SUCCESS )
	{
		if( !(checkNonAPI(result)) )
		{
			print_error( "Tspi_TPM_GetAttribUint32", result );
		}
		else
		{
			print_error_nonapi( "Tspi_TPM_GetAttribUint32", result );
		}
		goto done;
	}

	result = Tspi_TPM_Delegate_GetFamily(hTPM, familyID, &hFamily);
	if ( result != TSS_SUCCESS )
	{
		print_error( "Tspi_TPM_Delegate_GetFamily", (result) );
		goto done;
	}

	result = Tspi_GetAttribUint32(hFamily, TSS_TSPATTRIB_DELFAMILY_INFO,
			TSS_TSPATTRIB_DELFAMILYINFO_FAMILYID, &returnedID);
	if ( result != TSS_SUCCESS )
	{
		print_error( "Tspi_GetAttribUint32", (result) );
		goto done;
	}

	result = Testsuite_Transport_Final(hContext, hSigningKey);
	if ( result != TSS_SUCCESS )
	{
		if( !(checkNonAPI(result)) )
		{
			print_error( "Tspi_TPM_GetAttribUint32", result );
		}
		else
		{
			print_error_nonapi( "Tspi_TPM_GetAttribUint32", result );
		}
		goto done;
	}
	else
	{
		if (familyID != returnedID)
		{
			print_error( "Tspi_GetAttribUint32: Family IDs do not match", TSS_E_FAIL );
			result = 1;
			goto done;
		}
		else
		{
			print_success( function, result );
		}
	}

	print_end_test( function );
done:
	/* Invalidate the family to avoid resource exhaustion */
	if (hFamily != NULL_HDELFAMILY)
		Tspi_TPM_Delegate_InvalidateFamily(hTPM, hFamily);
	Tspi_Context_FreeMemory( hContext, NULL );
	Tspi_Context_Close( hContext );

	exit( result );
}
