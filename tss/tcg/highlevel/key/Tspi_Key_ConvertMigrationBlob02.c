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
 *	Tspi_Key_ConvertMigrationBlob02.c
 *
 * DESCRIPTION
 *	The purpose of this test is to migrate a key from one parent to
 *	another in the same key hierarchy.  After the migration, the key
 *	will be used to sign/verify some data which should show that the
 *	migration was successful.
 *
 * ALGORITHM
 *	Setup:
 *		Create Context
 *		Connect Context
 *		Get TPM Object
 *		Create hKey Object
 *		Get SRK Handle
 *		Get Policy Object (srk, tpm)
 *		Set secret (srk, tpm)
 *		Create Key (hkey, parent key, target key)
 *		Load key (parent key, hkey)
 *		Get Pub Key
 *		Set Attrib Data
 *		Authorize Migration Ticket
 *		LoadKeyByUUID
 *
 *	Test:	Call Key_ConvertMigrationBlob then if it is not a success
 *		make sure that it returns the proper return codes
 *
 *	Cleanup:
 *		Free memory associated with the context
 *		Close the hKey object
 *		Close the hParentStorageKey object
 *		Close the hTargetKey object
 *		Close the context.
 *		Print error/success message
 *
 * USAGE:	First parameter is --options
 *			-v or --version
 *		Second Parameter is the version of the test case to be run.
 *		This test case is currently only implemented for 1.1 and 1.2
 *
 * HISTORY
 *	Author:	Kathy Robertson
 *	Date:	June 2004
 *	Email:	klrobert@us.ibm.com
 *	Revisions: Kent Yoder <kyoder@users.sf.net>
 *		EJR, ejratl@gmail.com, 8/10/2006, 1.2 updates
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

main_v1_1(void)
{
	char *nameOfFunction = "Tspi_Key_ConvertMigrationBlob02";
	TSS_HCONTEXT hContext;
	TSS_HKEY hSRK;
	TSS_HKEY hKeyToMigrate, hDestChild, hDestParent;
	BYTE *MigTicket;
	UINT32 TicketLength;
	BYTE *randomData;
	UINT32 randomLength;
	UINT32 migBlobLength, bs, bs2;
	BYTE *migBlob, *b, *b2;
	TSS_RESULT result;
	TSS_HTPM hTPM;
	TSS_HPOLICY hPolicy, tpmUsagePolicy;

	print_begin_test(nameOfFunction);

	//Create Context
	result = Tspi_Context_Create(&hContext);
	if (result != TSS_SUCCESS) {
		print_error("Tspi_Context_Create ", result);
		exit(result);
	}
	//Connect Context
	result = Tspi_Context_Connect(hContext, get_server(GLOBALSERVER));
	if (result != TSS_SUCCESS) {
		print_error("Tspi_Context_Connect", result);
		Tspi_Context_Close(hContext);
		exit(result);
	}
	//Get TPM Object
	result = Tspi_Context_GetTpmObject(hContext, &hTPM);
	if (result != TSS_SUCCESS) {
		print_error("Tspi_Context_GetTpmObject", result);
		Tspi_Context_Close(hContext);
		exit(result);
	}
	//Load Key By UUID
	result = Tspi_Context_LoadKeyByUUID(hContext,
					    TSS_PS_TYPE_SYSTEM,
					    SRK_UUID, &hSRK);
	if (result != TSS_SUCCESS) {
		print_error("Tspi_Context_LoadKeyByUUID for hSRK", result);
		Tspi_Context_Close(hContext);
		exit(result);
	}
#ifndef TESTSUITE_NOAUTH_SRK
	//Get Policy Object
	result = Tspi_GetPolicyObject(hSRK, TSS_POLICY_USAGE, &hPolicy);
	if (result != TSS_SUCCESS) {
		print_error("Tspi_GetPolicyObject", result);
		Tspi_Context_Close(hContext);
		exit(result);
	}
	//Set Secret
	result = Tspi_Policy_SetSecret(hPolicy, TESTSUITE_SRK_SECRET_MODE,
				       TESTSUITE_SRK_SECRET_LEN,
				       TESTSUITE_SRK_SECRET);
	if (result != TSS_SUCCESS) {
		print_error("Tspi_Policy_SetSecret", result);
		Tspi_Context_Close(hContext);
		exit(result);
	}
#endif
	//Get Policy Object
	result =
	    Tspi_GetPolicyObject(hTPM, TSS_POLICY_USAGE, &tpmUsagePolicy);
	if (result != TSS_SUCCESS) {
		print_error("Tspi_GetPolicyObject", result);
		Tspi_Context_Close(hContext);
		exit(result);
	}
	//Set Secret
	result =
	    Tspi_Policy_SetSecret(tpmUsagePolicy,
				  TESTSUITE_OWNER_SECRET_MODE,
				  TESTSUITE_OWNER_SECRET_LEN,
				  TESTSUITE_OWNER_SECRET);
	if (result != TSS_SUCCESS) {
		print_error("Tspi_Policy_SetSecret", result);
		Tspi_Context_Close(hContext);
		exit(result);
	}
	//Create Object
	result =
	    Tspi_Context_CreateObject(hContext, TSS_OBJECT_TYPE_RSAKEY,
				      TSS_KEY_TYPE_STORAGE |
				      TSS_KEY_SIZE_2048 |
				      TSS_KEY_NO_AUTHORIZATION,
				      &hDestParent);
	if (result != TSS_SUCCESS) {
		print_error("Tspi_Context_CreateObject", result);
		Tspi_Context_Close(hContext);
		exit(result);
	}
	// hDestParent will be the new parent of hKeyToMigrate
	result = Tspi_Key_CreateKey(hDestParent, hSRK, 0);
	if (result != TSS_SUCCESS) {
		print_error("Tspi_Key_CreateKey", result);
		Tspi_Context_Close(hContext);
		exit(result);
	}
	//Create Object
	result =
	    Tspi_Context_CreateObject(hContext, TSS_OBJECT_TYPE_RSAKEY,
				      TSS_KEY_TYPE_SIGNING |
				      TSS_KEY_SIZE_2048 |
				      TSS_KEY_NO_AUTHORIZATION |
				      TSS_KEY_MIGRATABLE, &hKeyToMigrate);
	if (result != TSS_SUCCESS) {
		print_error("Tspi_Context_CreateObject", result);
		Tspi_Context_Close(hContext);
		exit(result);
	}
	result = Tspi_Context_CreateObject(hContext, TSS_OBJECT_TYPE_POLICY,
					TSS_POLICY_MIGRATION, &hPolicy);
	if (result != TSS_SUCCESS) {
		print_error("Tspi_Context_CreateObject", result);
		Tspi_Context_Close(hContext);
		exit(result);
	}
	//Set Secret
	result = Tspi_Policy_SetSecret(hPolicy, TESTSUITE_KEY_SECRET_MODE,
				       TESTSUITE_KEY_SECRET_LEN,
				       TESTSUITE_KEY_SECRET);
	if (result != TSS_SUCCESS) {
		print_error("Tspi_Policy_SetSecret", result);
		Tspi_Context_Close(hContext);
		exit(result);
	}
	//Assign migration policy to the key
	result = Tspi_Policy_AssignToObject(hPolicy, hKeyToMigrate);
	if (result != TSS_SUCCESS) {
		print_error("Tspi_Policy_AssignToObject", result);
		Tspi_Context_Close(hContext);
		exit(result);
	}
	//Create Key To Migrate
	result = Tspi_Key_CreateKey(hKeyToMigrate, hSRK, 0);
	if (result != TSS_SUCCESS) {
		print_error("Tspi_Key_CreateKey", result);
		Tspi_Context_Close(hContext);
		Tspi_Context_CloseObject(hContext, hKeyToMigrate);
		exit(result);
	}
	//Authorize Migration Ticket
	result =
	    Tspi_TPM_AuthorizeMigrationTicket(hTPM, hDestParent,
					      TSS_MS_MIGRATE,
					      &TicketLength, &MigTicket);
	if (result != TSS_SUCCESS) {
		print_error("Tpsi_TPM_AuthorizeMigrationTicket ", result);
		Tspi_Context_Close(hContext);
		exit(result);
	}

	/* hDestChild will hold the new migrated key */
	result =
	    Tspi_Context_CreateObject(hContext, TSS_OBJECT_TYPE_RSAKEY,
				      TSS_KEY_TYPE_SIGNING |
				      TSS_KEY_SIZE_2048 |
				      TSS_KEY_NO_AUTHORIZATION |
				      TSS_KEY_MIGRATABLE,
				      &hDestChild);
	if (result != TSS_SUCCESS) {
		print_error("Tspi_Context_CreateObject", result);
		Tspi_Context_Close(hContext);
		exit(result);
	}

	//Create Migration Blob
	result = Tspi_Key_CreateMigrationBlob(hKeyToMigrate, hSRK,
					      TicketLength, MigTicket,
					      &randomLength, &randomData,
					      &migBlobLength, &migBlob);
	if (result != TSS_SUCCESS) {
		print_error("Tspi_Key_CreateMigrationBlob", result);
		Tspi_Context_Close(hContext);
		exit(result);
	}
	result = Tspi_Key_LoadKey(hDestParent, hSRK);
	if (result != TSS_SUCCESS) {
		print_error("Tspi_Key_LoadKey", result);
		Tspi_Context_Close(hContext);
		exit(result);
	}

	result = Tspi_Key_ConvertMigrationBlob(hDestChild, hDestParent,
					       randomLength, randomData,
					       migBlobLength, migBlob);
	if (result != TSS_SUCCESS) {
		print_error("Tspi_Key_ConvertMigrationBlob", result);
		Tspi_Context_Close(hContext);
		exit(result);
	}

	// Load the newly migrated key
	result = Tspi_Key_LoadKey(hDestChild, hDestParent);
	if (result != TSS_SUCCESS) {
		print_error("Tspi_Key_LoadKey", result);
		Tspi_Context_Close(hContext);
		exit(result);
	}

	// Sign and verify with the migrated key to show that it's valid
	result = sign_and_verify(hContext, hDestChild);
	if (result != TSS_SUCCESS) {
		if (!checkNonAPI(result)) {
			print_error(nameOfFunction, result);
			print_end_test(nameOfFunction);
			Tspi_Context_FreeMemory(hContext, NULL);
			Tspi_Context_Close(hContext);
			exit(result);
		} else {
			print_error_nonapi(nameOfFunction, result);
			print_end_test(nameOfFunction);
			Tspi_Context_FreeMemory(hContext, NULL);
			Tspi_Context_Close(hContext);
			exit(result);
		}
	} else {
		print_success(nameOfFunction, result);
		print_end_test(nameOfFunction);
		Tspi_Context_FreeMemory(hContext, NULL);
		Tspi_Context_Close(hContext);
		exit(0);
	}
}
