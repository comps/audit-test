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
 **  FILE       : test_pciconfig_read.c
 **
 **  PURPOSE    : To test the pciconfig_read library call auditing.
 **
 **  DESCRIPTION: The test_pciconfig_read() function builds into the
 **  laus_test framework to verify that the Linux Audit System
 **  accurately logs both successful and erroneous execution of the
 **  "pciconfig_read" system call.
 **
 **  The success case reads a single byte from bus 0, device 0,
 **  function 0, at offset 0.  This will succeed because the bus,
 **  device, and function of the PCI controller is at this location.
 **
 **  The error case attempts to read from bus 0xFFFFFFFF, which is
 **  certain to be nonexistent in any system equipped with PCI
 **  buses.
 **
 **  HISTORY    :
 **    08/03 Originated by Michael A. Halcrow <mike@halcrow.us>
 **    03/04 Added exp_errno variable by D. Kent Soper <dksoper@us.ibm.com>
 **
 **********************************************************************/
   
#include "syscalls.h"

#ifdef __PPC

#include <asm-ppc/pci.h>

#define PCI_DEVFN(slot,func)    ((((slot) & 0x1f) << 3) | ((func) & 0x07))
#define PCI_SLOT(devfn)         (((devfn) >> 3) & 0x1f)
#define PCI_FUNC(devfn)         ((devfn) & 0x07)
   
int test_pciconfig_read( laus_data* dataPtr ) {
    
    int rc = 0;
    int exp_errno = EOPNOTSUPP;
    unsigned long bus;
    unsigned long dfn;
    unsigned long off; 
    unsigned long len;
    char buf[ 1 ];
   
    // Set the syscall-specific data
    printf5( "Setting laus_var_data.syscallData.code to %d\n", AUDIT_pciconfig_read );
    dataPtr->laus_var_data.syscallData.code = AUDIT_pciconfig_read;
     
    /**
     * Do as much setup work as possible right here
     */
    dataPtr->msg_euid = 0;
    dataPtr->msg_egid = 0;
    dataPtr->msg_fsuid = 0;
    dataPtr->msg_fsgid = 0;
    if( dataPtr->successCase ) {
	bus = 0;
	dfn = PCI_DEVFN( 0, 0 );
	off = 0;
	len = 1;
    } else {
	bus = 0xFFFFFFFF;
	dfn = PCI_DEVFN( 0, 0 );
	off = 0;
	len = 0;
    }

    // Set up audit argument buffer
    if( ( rc = auditArg5( dataPtr, 
			  AUDIT_ARG_IMMEDIATE, sizeof( bus ), &bus,
			  AUDIT_ARG_IMMEDIATE, sizeof( dfn ), &dfn,
			  AUDIT_ARG_IMMEDIATE, sizeof( off ), &off,
			  AUDIT_ARG_IMMEDIATE, sizeof( len ), &len,
			  AUDIT_ARG_POINTER, sizeof( char* ), buf ) ) != 0 ) {
	printf1( "Error setting up audit argument buffer\n" );
	goto EXIT;
    }
   
    // Do pre-system call work
    if ( (rc = preSysCall( dataPtr )) != 0 ) {
	printf1("ERROR: pre-syscall setup failed (%d)\n", rc);
	goto EXIT_CLEANUP;
    }
   
    // Execute system call
    dataPtr->laus_var_data.syscallData.result = 
	syscall( __NR_pciconfig_read, bus, dfn, off, len, (void*)buf );
   
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
