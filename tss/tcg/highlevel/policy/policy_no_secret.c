/*
 *
 *   Copyright (C) International Business Machines  Corp., 2006
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
 *	policy_no_secret.c
 *
 * DESCRIPTION
 *	This test will verify that TSS_E_POLICY_NO_SECRET gets returned
 *	whenever a policy is assigned to an object being used, but that policy's secret
 *	mode is TSS_SECRET_MODE_NONE.
 *
 * ALGORITHM
 *
 * USAGE:	First parameter is --options
 *			-v or --version
 *		Second Parameter is the version of the test case to be run.
 *		This test case is currently only implemented for 1.1
 *
 * HISTORY
 *	Kent Yoder, kyoder@users.sf.net
 *	Kent Yoder, kyoder@users.sf.net Updated 7/07
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

	char		*nameOfFunction = "policy_no_secret";
	TSS_HCONTEXT	hContext;
	TSS_HTPM	hTPM;
	TSS_FLAG	initFlags;
	TSS_HKEY	hKey;
	TSS_HKEY	hSRK;
	TSS_RESULT	result;
	TSS_BOOL	allowMaint = FALSE;
	BYTE		*randomData;
	TSS_HPOLICY	srkUsagePolicy, tpmUsagePolicy;

	print_begin_test(nameOfFunction);

#ifdef TESTSUITE_NOAUTH_SRK
	print_NA();
	print_end_test(nameOfFunction);
	exit(0);
#endif
		//Create Context
	result = Tspi_Context_Create(&hContext);
	if (result != TSS_SUCCESS) {
		print_error("Tspi_Context_Create", result);
		exit(result);
	}
		//Connect Context
	result = Tspi_Context_Connect(hContext, get_server(GLOBALSERVER));
	if (result != TSS_SUCCESS) {
		print_error("Tspi_Context_Connect", result);
		Tspi_Context_Close(hContext);
		exit(result);
	}

		//Load Key By UUID
	result = Tspi_Context_LoadKeyByUUID(hContext, TSS_PS_TYPE_SYSTEM, SRK_UUID, &hSRK);
	if (result != TSS_SUCCESS) {
		print_error("Tspi_Context_LoadKeyByUUID", result);
		Tspi_Context_CloseObject(hContext, hKey);
		Tspi_Context_Close(hContext);
		exit(result);
	}
		//Create Policy Object
	result = Tspi_Context_CreateObject(hContext, TSS_OBJECT_TYPE_POLICY, TSS_POLICY_USAGE,
					   &srkUsagePolicy);
	if (result != TSS_SUCCESS) {
		print_error("Tspi_Context_CreateObject", result);
		Tspi_Context_Close(hContext);
		exit(result);
	}
		//Set Secret mode to none
	result = Tspi_Policy_SetSecret(srkUsagePolicy, TSS_SECRET_MODE_NONE, 0, NULL);
	if (result != TSS_SUCCESS) {
		print_error("Tspi_Policy_SetSecret", result);
		Tspi_Context_CloseObject(hContext, hKey);
		Tspi_Context_Close(hContext);
		exit(result);
	}
		//Assign new policy w/o secret to the SRK object
	result = Tspi_Policy_AssignToObject(srkUsagePolicy, hSRK);
	if (result != TSS_SUCCESS) {
		print_error("Tspi_Policy_AssignToObject", result);
		Tspi_Context_Close(hContext);
		exit(result);
	}
		//Create child key Object
	result = Tspi_Context_CreateObject(hContext, TSS_OBJECT_TYPE_RSAKEY,
					   TSS_KEY_SIZE_512|TSS_KEY_TYPE_BIND|
					   TSS_KEY_NO_AUTHORIZATION, &hKey);
	if (result != TSS_SUCCESS) {
		print_error("Tspi_Context_CreateObject", result);
		Tspi_Context_Close(hContext);
		exit(result);
	}

		// Attempt to use the SRK w/o putting a secret in its policy
	result = Tspi_Key_CreateKey(hKey, hSRK, 0);
	if (TSS_ERROR_CODE(result) != TSS_E_POLICY_NO_SECRET) {
		print_error("Key creation w/o parent secret set", result);
		print_error(nameOfFunction, result);
		Tspi_Context_Close(hContext);
		exit(result);
	} else {
		print_success("Key creation w/o parent secret set", result);
	}

		//Set Secret
	result = Tspi_Policy_SetSecret(srkUsagePolicy, TESTSUITE_SRK_SECRET_MODE,
				TESTSUITE_SRK_SECRET_LEN, TESTSUITE_SRK_SECRET);
	if (result != TSS_SUCCESS) {
		print_error("Tspi_Policy_SetSecret", result);
		Tspi_Context_CloseObject(hContext, hKey);
		Tspi_Context_Close(hContext);
		exit(result);
	}

		// With a secret set, this should work OK
	result = Tspi_Key_CreateKey(hKey, hSRK, 0);
	if (result != TSS_SUCCESS) {
		print_error("Tspi_Key_CreateKey", result);
		Tspi_Context_Close(hContext);
		exit(result);
	}

		// Try an owner auth'd command w/o secret
	result = Tspi_Context_GetTpmObject(hContext, &hTPM);
	if (result != TSS_SUCCESS) {
		print_error("Tspi_Context_GetTpmObject", result);
		Tspi_Context_Close(hContext);
		exit(result);
	}

		//Get Policy Object
	result = Tspi_GetPolicyObject(hTPM, TSS_POLICY_USAGE, &tpmUsagePolicy);
	if (result != TSS_SUCCESS) {
		print_error("Tspi_GetPolicyObject", result);
		Tspi_Context_CloseObject(hContext, hKey);
		Tspi_Context_Close(hContext);
		exit(result);
	}
		//Set Secret mode to none
	result = Tspi_Policy_SetSecret(tpmUsagePolicy, TSS_SECRET_MODE_NONE, 0, NULL);
	if (result != TSS_SUCCESS) {
		print_error("Tspi_Policy_SetSecret", result);
		Tspi_Context_CloseObject(hContext, hKey);
		Tspi_Context_Close(hContext);
		exit(result);
	}

	result = Tspi_TPM_GetStatus(hTPM, TSS_TPMSTATUS_ALLOWMAINTENANCE, &allowMaint);
	if (TSS_ERROR_CODE(result) != TSS_E_POLICY_NO_SECRET) {
		print_error("Tspi_TPM_GetStatus w/o owner secret set", result);
		Tspi_Context_Close(hContext);
		exit(result);
	} else {
		print_success("TPM_GetStatus w/o owner secret set", result);
	}

		//Set Secret
	result = Tspi_Policy_SetSecret(tpmUsagePolicy, TESTSUITE_OWNER_SECRET_MODE,
				       TESTSUITE_OWNER_SECRET_LEN, TESTSUITE_OWNER_SECRET);
	if (result != TSS_SUCCESS) {
		print_error("Tspi_Policy_SetSecret", result);
		Tspi_Context_CloseObject(hContext, hKey);
		Tspi_Context_Close(hContext);
		exit(result);
	}
	result = Tspi_TPM_GetStatus(hTPM, TSS_TPMSTATUS_ALLOWMAINTENANCE, &allowMaint);
	if (result != TSS_SUCCESS) {
		print_error("Tspi_TPM_GetStatus", result);
		Tspi_Context_Close(hContext);
		exit(result);
	}

	print_success(nameOfFunction, result);
	print_end_test(nameOfFunction);
	Tspi_Context_Close(hContext);
	exit(result);
}
