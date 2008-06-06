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
 *	Tspi_Data_Seal05.c
 *
 * DESCRIPTION
 *	This test will set a pcr composite object so that all the pcrs are
 *	selected and set to the current values of the PCRs in the TPM. An
 *	encrypted data object will then be Sealed and Unsealed, checking
 *	that the data is the same after being unsealed.
 *
 * ALGORITHM
 *	Setup:
 *              Create Context
 *              Connect Context
 *              Create RSAKEY Object
 *              Load Key By UUID for SRK
 *              GetPolicy
 *              SetSecret
 *              Create Key
 *              Load Key
 *              Create Seal Data Object
 *
 *	Test:
 *              Seal Data (data smaller than RSA key size)
 *              Unseal Data
 *              Check that Data returned matches original data
 *
 *	Cleanup:
 *		Free memory associated with the context
 *		Close hKey object
 *              Close Encrypted Data Object
 *		Close context
 *		Print error/success message
 *
 * USAGE:	First parameter is --options
 *			-v or --version
 *		Second Parameter is the version of the test case to be run.
 *		This test case is currently only implemented for 1.1
 *
 * HISTORY
 *	Author:	Kent Yoder <kyoder@users.sf.net>
 *
 * RESTRICTIONS
 *	None.
 */

#include "common.h"


int main(int argc, char **argv)
{
	char version;

	version = parseArgs( argc, argv );
	if (version)
		main_v1_1();
	else
		print_wrongVersion();
}

main_v1_1(void){

	char		*function = "Tspi_Data_Unseal05";
	TSS_FLAG	initFlags;
	TSS_HKEY	hKey;
	TSS_HKEY	hSRK;
	TSS_HCONTEXT	hContext;
	TSS_HTPM	hTPM;
	TSS_RESULT	result;
	initFlags	= TSS_KEY_TYPE_STORAGE | TSS_KEY_SIZE_2048  |
			  TSS_KEY_VOLATILE | TSS_KEY_AUTHORIZATION |
			  TSS_KEY_NOT_MIGRATABLE;
	TSS_HENCDATA	hEncData;
	TSS_HKEY	hEncKey;
	TSS_HPOLICY	phPolicy, hEncPolicy;
	TSS_HPCRS	hPcrs;
	UINT32		dataLength=32, ulPcrValueLength, subCap, subCapLength;
	UINT32		dataUnsealedLength=0, pulRespDataLength, i, numPcrs;
	BYTE		*rgbDataToSeal, *pNumPcrs;
	BYTE		*rgbDataUnsealed, *rgbPcrValue;
	int		exitCode=0;

	print_begin_test(function);

	rgbDataToSeal = "0123456789ABCDEF0123456789ABCDEF";

	if ((result = connect_load_all(&hContext, &hSRK, &hTPM)))
		return result;

	if ((result = create_load_key(hContext, initFlags, hSRK, &hKey)))
		return result;

	//Create Encrypted Data Object
	result = Tspi_Context_CreateObject(hContext,
			TSS_OBJECT_TYPE_ENCDATA,
			TSS_ENCDATA_SEAL,
			&hEncData);
	if (result != TSS_SUCCESS) {
		print_error("Tspi_Context_CreateObject", result);
		Tspi_Context_FreeMemory(hContext, NULL);
		Tspi_Context_Close(hContext);
		exit(result);
	}

	if ((result = set_secret(hContext, hEncData, &hEncPolicy)))
		return result;

	//Create Encrypted Data Object
	result = Tspi_Context_CreateObject(hContext,
			TSS_OBJECT_TYPE_PCRS,
			0,
			&hPcrs);
	if (result != TSS_SUCCESS) {
		print_error("Tspi_Context_CreateObject", result);
		Tspi_Context_FreeMemory(hContext, NULL);
		Tspi_Context_Close(hContext);
		exit(result);
	}
	print_success("Create Encrypted Data Object", result);

	subCap = TSS_TPMCAP_PROP_PCR;
	subCapLength = sizeof(UINT32);

	result = Tspi_TPM_GetCapability( hTPM, TSS_TPMCAP_PROPERTY,
			subCapLength, (BYTE *)&subCap,
			&pulRespDataLength,
			&pNumPcrs );
	if ( result != TSS_SUCCESS )
	{
		print_error( "Tspi_TPM_GetCapability", result );
		Tspi_Context_FreeMemory( hContext, NULL );
		Tspi_Context_Close( hContext );
		exit( result );
	}

	numPcrs = *(UINT32 *)pNumPcrs;

	for (i = 0; i < numPcrs; i++) {
		result = Tspi_TPM_PcrRead( hTPM, i, &ulPcrValueLength, &rgbPcrValue );
		if ( result != TSS_SUCCESS )
		{
			print_error( "Tspi_TPM_PcrRead", result );
			Tspi_Context_FreeMemory( hContext, NULL );
			Tspi_Context_Close( hContext );
			exit( result );
		}

		result = Tspi_PcrComposite_SetPcrValue(hPcrs, i, ulPcrValueLength,
				rgbPcrValue);
		if ( result != TSS_SUCCESS )
		{
			print_error( "Tspi_PcrComposite_SetPcrValue", result );
			Tspi_Context_FreeMemory( hContext, NULL );
			Tspi_Context_Close( hContext );
			exit( result );
		}
	}

	//Seal data
	result = Tspi_Data_Seal(hEncData, 
			hKey, 
			dataLength, 
			rgbDataToSeal, hPcrs);
	if (result != TSS_SUCCESS) {
		print_error_nonapi(function, result);
		print_end_test(function);
		Tspi_Context_FreeMemory(hContext, NULL);
		Tspi_Context_CloseObject(hContext, hKey);
		Tspi_Context_CloseObject(hContext, hEncData);
		Tspi_Context_Close(hContext);
		exit(result);
	}
	else {
		print_success("Tspi_Data_Seal", result);
		exitCode = 0;
	}

	//Unseal data
	result = Tspi_Data_Unseal(hEncData, 
			hKey,
			&dataUnsealedLength,
			&rgbDataUnsealed );
	if (result != TSS_SUCCESS) {
		print_error_nonapi(function, result);
		print_end_test(function);
		Tspi_Context_FreeMemory(hContext, NULL);
		Tspi_Context_CloseObject(hContext, hKey);
		Tspi_Context_CloseObject(hContext, hEncData);
		Tspi_Context_Close(hContext);
		exit(result);
	}
	else {
		print_success("Tspi_Data_Unseal", result);
		exitCode = 0;
	}


	//compare Original Data with Data returned by Unseal
	//First compare size
	if (dataLength == dataUnsealedLength)
	{
		//Size correct, check if data matches
		print_success("Unseal data length matches length of original data", result);
		if (strcmp(rgbDataToSeal, rgbDataUnsealed) == 0)
		{
			//strings match
			print_success("Unseal data matches original data!", result);
			exitCode = 0;
		}
		else {
			//string size the same but strings don't match
			print_error("Unseal data does NOT match original data", result);
			exitCode = 1;
		}
	}
	else {
		//string sizes do not match, so strings do not match
		print_error("Unseal data size does NOT match original data size", result);
		exitCode = 1;
	}

	print_end_test(function);
	Tspi_Context_FreeMemory(hContext, NULL);
	Tspi_Context_CloseObject(hContext, hKey);
	Tspi_Context_CloseObject(hContext, hEncData);
	Tspi_Context_Close(hContext);
	exit(exitCode);

}
