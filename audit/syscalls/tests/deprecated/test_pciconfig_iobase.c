/**********************************************************************
 **   Copyright (C) International Business Machines  Corp., 2003
 **
 **   This program is free software;  you can redistribute it and/or modify
 **   it under the terms of the GNU General Public License as published by
 **   the Free Software Foundation; either version 2 of the License, or
 **   (at your option) any later version.
 **
 **   This program is distributed in the hope that it will be useful,
 **   but WITHOUT ANY WARRANTY;  without even the implied warranty of
 **   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See
 **   the GNU General Public License for more details.
 **
 **   You should have received a copy of the GNU General Public License
 **   along with this program;  if not, write to the Free Software
 **   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 **
 **
 **
 **  FILE       : test_pciconfig_iobase.c
 **
 **  PURPOSE    : To test the pciconfig_iobase library call auditing.
 **
 **  DESCRIPTION: The test_pciconfig_iobase() function builds into the
 **  laus_test framework to verify that the Linux Audit System
 **  accurately logs both successful and erroneous execution of the
 **  "pciconfig_iobase" system call.
 **
 **  In the success case, the which, bus, and devfn parameters are all
 **  set to ``0''.  This is assigned to the PCI controller device, and
 **  so it is valid.
 **
 **  In the failure case, the which parameter is set to 0xFFFFFFFF,
 **  which is an invalid paramerter.  This causes an ``operation not
 **  supported'' error.
 **
 **  HISTORY    :
 **    08/03 Originated by Michael A. Halcrow <mike@halcrow.us>
 **    03/04 Added exp_errno variable by D. Kent Soper <dksoper@us.ibm.com>
 **
 **********************************************************************/
   
#include "syscalls.h"

#ifdef __PPC
   
int test_pciconfig_iobase( laus_data* dataPtr ) {
    
    int rc = 0;
    int exp_errno = EOPNOTSUPP;
    long which;
    unsigned long bus;
    unsigned long devfn;
   
    // Set the syscall-specific data
    printf5( "Setting laus_var_data.syscallData.code to %d\n", AUDIT_pciconfig_iobase );
    dataPtr->laus_var_data.syscallData.code = AUDIT_pciconfig_iobase;
     
    /**
     * Do as much setup work as possible right here
     */
    if( dataPtr->successCase ) {
	which = 0;
	bus = 0;
	devfn = 0;
    } else {
	which = 0xFFFFFFFF;
	bus = 0;
	devfn = 0;
    }

    // Set up audit argument buffer
    if( ( rc = auditArg3( dataPtr, 
			  AUDIT_ARG_IMMEDIATE, sizeof( which ), &which,
			  AUDIT_ARG_IMMEDIATE, sizeof( bus ), &bus,
			  AUDIT_ARG_IMMEDIATE, sizeof( devfn ), &devfn ) ) != 0 ) {
	printf1( "Error setting up audit argument buffer\n" );
	goto EXIT;
    }
   
    // Do pre-system call work
    if ( (rc = preSysCall( dataPtr )) != 0 ) {
	printf1("ERROR: pre-syscall setup failed (%d)\n", rc);
	goto EXIT_CLEANUP;
    }
   
    // Execute system call
    dataPtr->laus_var_data.syscallData.result = syscall( __NR_pciconfig_iobase, which, bus, devfn );
   
    // Do post-system call work
    if( ( rc = postSysCall( dataPtr, errno, -1, exp_errno ) ) != 0 ) {
	printf1( "ERROR: post-syscall setup failed (%d)\n", rc );
	goto EXIT_CLEANUP;
    }
   
     
   
 EXIT_CLEANUP:
    /**
     * Do cleanup work here
     */
   
 EXIT:
    printf5( "Returning from test\n" );
    return rc;
}

#endif
