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
 *	Tspi_Hash_VerifySignature01.c
 *
 * DESCRIPTION
 *	This test will verify that Tspi_Hash_VerifySignature
 *		returns TSS_SUCCESS.
 *
 * ALGORITHM
 *	Setup:
 *		Create Context
 *		Connect Context
 *		Get SRK Handle
 *		Get Policy Object
 *		Set Secret
 *		Create Signing Key Object
 *		Create Signing Key
 *		Load Key
 *		Create Hash Object
 *		Set Hash Value
 *		Sign Hash
 *		Create Signature Pub Key Object
 *		Create Signature Pub Key
 *		Load Key
 *		Get Pub Key
 *		Set Attrib Data
 *
 *	Test:
 *		Call Hash_VerifySignature then if it does not succeed
 *		Make sure that it returns the proper return codes
 *		Print results
 *
 *	Cleanup:
 *		Unregister system key
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
 *      Kent Yoder, shpedoikal@gmail.com, 11/04
 *	EJR, ejratl@gmail.com, 8/10/2006, 1.2 updates
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
	char *function = "Tspi_Hash_VerifySignature01";
	TSS_HCONTEXT hContext;
	TSS_HKEY hSRK;
	TSS_HKEY hMSigningKey;
	TSS_HKEY hSignaturePublicKey;
	TSS_HKEY hKey;
	TSS_UUID SRKUUID = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1 };
	TSS_UUID migratableSignUUID = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 2 };
	TSS_HHASH hHash;
	BYTE *prgbSignature;
	UINT32 pulSignatureLength;
	UINT32 signaturePublicKeyDataLength;
	BYTE *signaturePublicKeyData;
	TSS_RESULT result;
	TSS_HPOLICY srkUsagePolicy;
	TSS_FLAG initFlags = TSS_KEY_TYPE_SIGNING | TSS_KEY_SIZE_2048 |
	    TSS_KEY_VOLATILE | TSS_KEY_NO_AUTHORIZATION |
	    TSS_KEY_NOT_MIGRATABLE;

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
	//Load Key By UUID
	result = Tspi_Context_LoadKeyByUUID(hContext, TSS_PS_TYPE_SYSTEM,
					    SRK_UUID, &hSRK);
	if (result != TSS_SUCCESS) {
		print_error("Tspi_Context_LoadKeyByUUID (hSRK)", result);
		Tspi_Context_FreeMemory(hContext, NULL);
		Tspi_Context_Close(hContext);
		exit(result);
	}
#ifndef TESTSUITE_NOAUTH_SRK
	//Get Policy Object
	result = Tspi_GetPolicyObject(hSRK, TSS_POLICY_USAGE,
				      &srkUsagePolicy);
	if (result != TSS_SUCCESS) {
		print_error("Tspi_GetPolicyObject", result);
		Tspi_Context_FreeMemory(hContext, NULL);
		Tspi_Context_Close(hContext);
		exit(result);
	}
	//Set Secret
	result =
	    Tspi_Policy_SetSecret(srkUsagePolicy,
				  TESTSUITE_SRK_SECRET_MODE,
				  TESTSUITE_SRK_SECRET_LEN,
				  TESTSUITE_SRK_SECRET);
	if (result != TSS_SUCCESS) {
		print_error("Tspi_Policy_SetSecret", result);
		Tspi_Context_FreeMemory(hContext, NULL);
		Tspi_Context_Close(hContext);
		exit(result);
	}
#endif
	//Create Signing Key
	result =
	    Tspi_Context_CreateObject(hContext, TSS_OBJECT_TYPE_RSAKEY,
				      TSS_KEY_SIZE_2048 |
				      TSS_KEY_TYPE_SIGNING |
				      TSS_KEY_NO_AUTHORIZATION,
				      &hMSigningKey);
	if (result != TSS_SUCCESS) {
		print_error("Tspi_Context_CreateObject (signing key)",
			    result);
		Tspi_Context_FreeMemory(hContext, NULL);
		Tspi_Context_Close(hContext);
		exit(result);
	}

	result = Tspi_Key_CreateKey(hMSigningKey, hSRK, 0);
	if (result != TSS_SUCCESS) {
		print_error("Tspi_Key_CreateKey (signing key)", result);
		Tspi_Context_FreeMemory(hContext, NULL);
		Tspi_Context_Close(hContext);
		exit(result);
	}

	result = Tspi_Key_LoadKey(hMSigningKey, hSRK);
	if (result != TSS_SUCCESS) {
		print_error("Tspi_Key_LoadKey (hMSigningKey)", result);
		Tspi_Context_FreeMemory(hContext, NULL);
		Tspi_Context_Close(hContext);
		exit(result);
	}
	// create hash
	result = Tspi_Context_CreateObject(hContext, TSS_OBJECT_TYPE_HASH,
					   TSS_HASH_SHA1, &hHash);
	if (result != TSS_SUCCESS) {
		print_error("Tspi_Context_CreateObject (hash)", result);
		Tspi_Context_FreeMemory(hContext, NULL);
		Tspi_Context_Close(hContext);
		exit(result);
	}

	result = Tspi_Hash_SetHashValue(hHash, 20, "Je pense, danc je s");
	if (result != TSS_SUCCESS) {
		print_error("Tspi_Hash_SetHashValue", result);
		Tspi_Context_FreeMemory(hContext, NULL);
		Tspi_Context_Close(hContext);
		exit(result);
	}

	result = Tspi_Hash_Sign(hHash, hMSigningKey, &pulSignatureLength,
				&prgbSignature);
	if (result != TSS_SUCCESS) {
		print_error("Tspi_Hash_Sign", result);
		Tspi_Context_FreeMemory(hContext, NULL);
		Tspi_Context_Close(hContext);
		exit(result);
	}

	result = Tspi_Hash_VerifySignature(hHash, hMSigningKey,
					   pulSignatureLength,
					   prgbSignature);
	if (result != TSS_SUCCESS) {
		if (!(checkNonAPI(result))) {
			print_error(function, result);
		} else {
			print_error_nonapi(function, result);
		}
	} else {
		print_success(function, result);
	}

	print_end_test(function);
#if 0
	Tspi_Context_UnregisterKey(hContext, TSS_PS_TYPE_SYSTEM,
				   migratableSignUUID, &hMSigningKey);
#endif
	Tspi_Context_FreeMemory(hContext, NULL);
	Tspi_Context_Close(hContext);
	exit(result);
}
