/**********************************************************************
 **   Copyright (C) International Business Machines  Corp., 2003
 **   Some code to test the SIOCSIFENCAP ioctl copied and adapted from
 **   rrouted.c, which is in the public domain, written by Olaf Titz
 **   <olaf@bigred.inka.de>, Sept. 1998.
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
 **  FILE       : test_ioctl.c
 **
 **  PURPOSE    : To test the ioctl library call auditing.
 **
 **  DESCRIPTION: The test_ioctl() function builds into the
 **  laus_test framework to verify that the Linux Audit System
 **  accurately logs both successful and erroneous execution of
 **  various "ioctl" system calls.
 **
 **  The success cases vary depending on the specific request passed
 **  via ioctl.  The ethertap module is inserted into the kernel and
 **  the tap0 network device is assigned the address 192.168.42.42
 **  before any of the success cases are run.  A new datagram socket
 **  is created, data structures are populated, and then the tests
 **  begin.  Each test uses the rc = runIoctlTest() utility function to set
 **  up the audit arguments, perform the pre-syscall work, make the
 **  system call, and perform the post-syscall evaluation.
 **
 **  The error cases focus on providing invalid parameters in the
 **  ioctl calls.  Many tests invoke the ENODEV errno, and many others
 **  invoke the EINVAL errno.  One test invokes the ESRCH errno.
 **  These tests are run without the ethertap module loaded, and they
 **  are given a bogus network device name.
 **
 **  HISTORY    :
 **    08/03 Originated by Dan Jones (danjones@us.ibm.com)
 **    08/03 Furthered by Michael A. Halcrow <mike@halcrow.us>
 **    03/04 Added exp_errno variable by D. Kent Soper <dksoper@us.ibm.com>
 **
 **********************************************************************/

#include "syscalls.h"
#include <linux/ip.h>
#include <linux/types.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <net/if_arp.h>
#include <net/route.h>
#include <stdio.h>
#include <linux/sockios.h>
#include <linux/if_tunnel.h>
//#include <linux/ethtool.h>

#ifndef SIOCSIFNAME
#define SIOCSIFNAME  0x8923
#endif

static void
in_set(struct sockaddr *ap, u_int32_t addr)
{
  struct sockaddr_in *sin;

  sin = (struct sockaddr_in *) ap;
  memset(sin, 0, sizeof(*sin));
  sin->sin_family = AF_INET;
  sin->sin_addr.s_addr = addr;
}

static void
hw_set(struct sockaddr *ap, const unsigned char *hwaddr, size_t len)
{
  ap->sa_family = AF_UNSPEC;
  memcpy(ap->sa_data, hwaddr, len);
}

// From rrouted.c
int openpty( int *mFileDes ) {
  static const char* majorNum = "pqrstuvw";
  static const char* minorNum = "0123456789abcdef";
  const char *p, *q;
  int e;
  char b[ 16 ];

  for( p = majorNum; *p; p++ ) {
    for( q = minorNum; *q; q++ ) {
      sprintf( b, "/dev/pty%c%c", *p, *q );
      if( ( (*mFileDes) = open( b, O_RDWR|O_NOCTTY ) ) >= 0 ) {
	b[ 5 ] = 't';
	dprintf( ( "Using %s\n", b ) );
	if( ( e = open( b, O_RDWR|O_NOCTTY ) ) >= 0 )
	  return e;
	printf( "open %s: %m", b );
	close( *mFileDes );
      }
    }
  }
  errno = EXFULL;
  return -1;
}

struct in_addr dummynet;
struct in_addr target;
struct in_addr mask;

int slipfd;   

int runIoctlTest( laus_data *dataPtr, int d, int request, 
		  void* data, int pointerSize, int targetErrno ) {

  int rc = 0;
  char socket_id[25];

  snprintf(socket_id, 24, "[sock:af=%d,type=%d]", PF_INET, SOCK_DGRAM);

  // Set up audit argument buffer
  if( ( rc = auditArg3( dataPtr, 
 			AUDIT_ARG_PATH, strlen(socket_id), socket_id,
 			AUDIT_ARG_IMMEDIATE, sizeof( int ), &request,
 			( data == NULL ? AUDIT_ARG_NULL : AUDIT_ARG_POINTER ),
			pointerSize, data ) ) != 0 ) {
    printf1( "Error setting up audit argument buffer\n" );
    return rc;
  }
   
  // Do pre-system call work
  if( (rc = preSysCall( dataPtr ) ) != 0 ) {
    printf1("ERROR: pre-syscall setup failed (%d)\n", rc);
    return rc;
  }
   
  // Execute system call
  dataPtr->laus_var_data.syscallData.result = syscall( __NR_ioctl, d, request, data );
   
  // Do post-system call work
  if( ( rc = postSysCall( dataPtr, errno, -1, targetErrno ) ) != 0 ) {
    printf1( "ERROR: post-syscall setup failed (%d)\n", rc );
    return rc;
  }

  return 0;

}

int rc = 0;

struct in_addr addr;
const char* ifname;
struct ifreq ifr;
struct rtentry rt;
struct arpreq arp;
int fd;

void cleanup_ioctl_test() {

  RUNCOMMAND( "/sbin/ifconfig tap0 down" );
  RUNCOMMAND( "rmmod bridge" );
  RUNCOMMAND( "rmmod ethertap" );

}


int setup_ioctl_test( laus_data* dataPtr ) {

  rc = 0;

  // Set the syscall-specific data
  printf5( "Setting laus_var_data.syscallData.code to %d\n", AUDIT_ioctl );
  dataPtr->laus_var_data.syscallData.code = AUDIT_ioctl;

  // Setup
  dataPtr->msg_euid = 0;
  dataPtr->msg_egid = 0;
  dataPtr->msg_fsuid = 0;
  dataPtr->msg_fsgid = 0;

  // Insmod ethertap
  // TODO: Check to see if ethertap is already loaded
  // TODO: Check to see if bridge is already loaded
  // ALTERNATE: "modprobe ethertap"?
  RUNCOMMANDORDIE( "insmod /lib/modules/`uname -r`/kernel/drivers/net/ethertap.o" );
  // ALTERNATE: "modprobe bridge"?
  RUNCOMMANDORDIE( "insmod /lib/modules/`uname -r`/kernel/net/bridge/bridge.o" );
  // TODO: Check to see if tap0 is already active, or if 192.168.42.42 is already taken
  RUNCOMMANDORDIE( "/sbin/ifconfig tap0 192.168.42.42" );

  if( dataPtr->successCase ) {
    ifname = "tap0";
    if( !inet_aton( "192.168.42.42", &addr ) ) {
      printf1( "Cannot set addr to 192.168.42.42\n" );
      cleanup_ioctl_test();
      return -1;
    }
  } else {
    ifname = "trateotu42";
  }

  if( ( fd = socket( PF_INET, SOCK_DGRAM, 0 ) ) < 0 ) {
    perror( "socket" );
    return 1;
  }
  strcpy(ifr.ifr_name, ifname);

 EXIT_CLEANUP:

  return rc;

}

/* This must be supported on the target device (eth0). */
/* Works on my machine :-) */
int test_ioctl_SIOCSMIIREG( laus_data* dataPtr ) {

  int rc = 0;

  if( ( rc = setup_ioctl_test( dataPtr ) ) ) {
    printf1( "Error setting up for ioctl test\n" );
    return rc;
  }

  if( dataPtr->successCase ) {
    strcpy( ifr.ifr_name, "eth0" );
    printf( "Now testing ioctl SIOCGMIIREG\n" );
    if( rc = runIoctlTest( dataPtr, fd, SIOCGMIIREG, &ifr, sizeof( struct ifreq ), EOPNOTSUPP ) != 0 ) {
      printf1( "Error running ioctl test\n" );
      rc = -1;
    goto EXIT_CLEANUP;
    }
  } else {
    strcpy( ifr.ifr_name, "tap0" );
  }

  printf( "Now testing ioctl SIOCSMIIREG\n" );
  if( rc = runIoctlTest( dataPtr, fd, SIOCSMIIREG, &ifr, sizeof( struct ifreq ), EOPNOTSUPP ) != 0 ) {
    printf1( "Error running ioctl test\n" );
    rc = -1;
    goto EXIT_CLEANUP;
  }

 EXIT_CLEANUP:
  cleanup_ioctl_test();

  return rc;

}

in_addr_t getIfaddr(char *ifName) {

  struct ifreq ifreq;
  int sock;
  
  ifreq.ifr_addr.sa_family = AF_INET;
  
  if( ( rc = sock = socket( ifreq.ifr_addr.sa_family, SOCK_DGRAM, 0 ) ) < 0 ) {
    printf1( "Error creating socket\n" );
    return 0;
  }

  strncpy( ifreq.ifr_name, ifName, sizeof( ifreq.ifr_name ) );

  if( ioctl( sock, SIOCGIFADDR, &ifreq) == -1 ) {
    printf1( "Error getting IFADDR\n" );    
    return 0;
  }

  close( sock );
  
  return ( ( struct sockaddr_in* )&ifreq.ifr_addr )->sin_addr.s_addr;
}

/* No success case (errno 22) */
/* This appears to be an IPv6-only call */
/* MH: Implemented via userland utility ifconfig; still needs verification */
int test_ioctl_SIOCDIFADDR( laus_data* dataPtr ) {

  int rc = 0;
  int exp_errno = EINVAL;
  int s;
  struct ifreq ifreq;
  struct sockaddr_in *in;

  //  if( dataPtr->successCase ) {
  //    rc = SKIP_TEST_CASE;
  //    goto EXIT;
  //  }

  if( ( rc = setup_ioctl_test( dataPtr ) ) ) {
    printf1( "Error setting up for ioctl test\n" );
    return rc;
  }

  if( dataPtr->successCase ) {
    RUNCOMMANDORDIE( "/sbin/ifconfig tap0 down" );
    RUNCOMMANDORDIE( "/sbin/ifconfig tap0 inet6 up" );
    RUNCOMMANDORDIE( "/sbin/ifconfig tun0 inet6 add 3ffe:ffff:0:f101::1/64" );
  }

  s = socket( PF_INET, SOCK_STREAM, 0 );
  if( s == -1 ) {
    perror("socket()");
    return 1;
  }

  memset( &ifreq, 0, sizeof( ifreq ) );

  strncpy( ifreq.ifr_name, "tap0", sizeof(ifreq.ifr_name) );

  in = (struct sockaddr_in *) &ifreq.ifr_addr;
  in->sin_family = AF_INET;

  /*
   * let get_ifaddr() find the address of the interface
   */
  in->sin_addr.s_addr = getIfaddr(ifreq.ifr_name);

  //  if( ioctl( fd, SIOCSIFADDR, &ifreq, sizeof( struct ifreq ) ) < 0 ) {
  //    printf1( "Error setting ifaddr\n" );
  //    goto EXIT_CLEANUP;
  //  }

  if( dataPtr->successCase ) {
    
    if( ( rc = auditArgNil( dataPtr ) ) != 0 ) {
      printf1( "Error setting up audit argument buffer\n" );
      return rc;
    }
    
    // Do pre-system call work
    if( (rc = preSysCall( dataPtr ) ) != 0 ) {
      printf1("ERROR: pre-syscall setup failed (%d)\n", rc);
      return rc;
    }
    
    // (indirectly) execute system call
    RUNCOMMANDORDIE( "/sbin/ifconfig tun0 inet6 del 3ffe:ffff:0:f101::1/64" );
    
    // Do post-system call work
    if( ( rc = postSysCall( dataPtr, errno, -1, exp_errno ) ) != 0 ) {
      printf1( "ERROR: post-syscall setup failed (%d)\n", rc );
      return rc;
    }
    
  } else {
    printf( "Now testing ioctl SIOCDIFADDR\n" );
    if( rc = runIoctlTest( dataPtr, fd, SIOCDIFADDR, &ifreq, sizeof( struct ifreq ), EINVAL ) != 0 ) {
      printf1( "Error running ioctl test\n" );
      rc = -1;
      goto EXIT_CLEANUP;
    }
  }

 EXIT_CLEANUP:
  cleanup_ioctl_test();

 EXIT:

  return rc;

}

/* Error case only: errno 22 */
/* This does not appear to be supported in the kernel */
/* From man rarp:
   This  program  is obsolete.  From version 2.3, the Linux kernel no longer 
   contains RARP support.
*/
int test_ioctl_SIOCDRARP( laus_data* dataPtr ) {

  int rc = 0;

  if( dataPtr->successCase ) {
    rc = SKIP_TEST_CASE;
    goto EXIT;
  }  

  if( ( rc = setup_ioctl_test( dataPtr ) ) ) {
    printf1( "Error setting up for ioctl test\n" );
    return rc;
  }

  hw_set(&ifr.ifr_hwaddr, "\xa5\xa5\xa5\xa5\xa5\xa5", 6);
  ifr.ifr_hwaddr.sa_family = ARPHRD_ETHER;
  in_set(&arp.arp_pa, addr.s_addr);
  hw_set(&arp.arp_ha, "\xa5\xa5\xa5\xa5\xa5\xa5", 6);
  /* arp.arp_ha.sa_family = ARPHRD_ETHER; */
  arp.arp_flags = ATF_PERM|ATF_PUBL|ATF_COM;
  strcpy(arp.arp_dev, ifname);

  if( ioctl( fd, SIOCSRARP, &arp, sizeof( struct arpreq ) ) == -1 ) {
    printf1( "Error setting RARP entry\n" );
    goto EXIT_CLEANUP;
  }

  printf( "Now testing ioctl SIOCDRARP\n" );
  if( rc = runIoctlTest( dataPtr, fd, SIOCDRARP, &arp, sizeof( struct arpreq ), ENODEV ) != 0 ) {
    printf1( "Error running ioctl test\n" );
    goto EXIT_CLEANUP;
  }

 EXIT_CLEANUP:
  cleanup_ioctl_test();

 EXIT:
  return rc;

}

/* Use ``ip'' command to call indirectly in success case */
int test_ioctl_SIOCCHGTUNNEL( laus_data* dataPtr ) {

  int rc = 0;
  int exp_errno = EFAULT;
  char socket_id[25];
  int request = SIOCCHGTUNNEL;

  if( dataPtr->successCase ) {

    RUNCOMMANDORDIE( "/sbin/ip tunnel add tunl42 mode ipip remote 192.168.43.43" );
  
    if( ( rc = auditArgNil( dataPtr ) ) != 0 ) {
      printf1( "Error setting up audit argument buffer\n" );
      goto EXIT_CLEANUP;
    }
   
    // Do pre-system call work
    if( (rc = preSysCall( dataPtr ) ) != 0 ) {
      printf1("ERROR: pre-syscall setup failed (%d)\n", rc);
      goto EXIT_CLEANUP;
    }
   
    // Execute system call
    // TODO: Get pid
    RUNCOMMANDORDIE( "/sbin/ip tunnel change tunl42 mode ipip remote 192.168.43.44 ttl 32" );

    // Do post-system call work
    if( ( rc = postSysCall( dataPtr, errno, -1, exp_errno ) ) != 0 ) {
      printf1( "ERROR: post-syscall setup failed (%d)\n", rc );
      goto EXIT;
    }
  
  EXIT_CLEANUP:
    RUNCOMMANDORDIE( "/sbin/ip tunnel del tunl42" );
  EXIT:
    return rc;

  } else {

    if( ( rc = setup_ioctl_test( dataPtr ) ) ) {
      printf1( "Error setting up for ioctl test\n" );
      return rc;
    }
    
    printf( "Now testing ioctl SIOCCHGTUNNEL\n" );
    if( rc = runIoctlTest( dataPtr, fd, SIOCCHGTUNNEL, &ifr, sizeof( struct ifreq ), ENODEV ) != 0 ) {
      printf1( "Error running ioctl test\n" );
      rc = -1;
      goto EXIT_CLEANUP2;
    }
    
  EXIT_CLEANUP2:
    cleanup_ioctl_test();
    
    return rc;
    
  }
  
}

/* Use ``ip'' command to call indirectly in success case */
int test_ioctl_SIOCDELTUNNEL( laus_data* dataPtr ) {

  int rc = 0;
  int exp_errno = EFAULT;
  char socket_id[25];
  int request = SIOCDELTUNNEL;

  if( dataPtr->successCase ) {

    RUNCOMMANDORDIE( "/sbin/ip tunnel add tunl42 mode ipip remote 192.168.43.43" );
  
    if( ( rc = auditArgNil( dataPtr ) ) != 0 ) {
      printf1( "Error setting up audit argument buffer\n" );
      goto EXIT_CLEANUP;
    }
   
    // Do pre-system call work
    if( (rc = preSysCall( dataPtr ) ) != 0 ) {
      printf1("ERROR: pre-syscall setup failed (%d)\n", rc);
      goto EXIT_CLEANUP;
    }
   
    // Execute system call
    // TODO: Get pid
    RUNCOMMANDORDIE( "/sbin/ip tunnel del tunl42" );
   
    // Do post-system call work
    if( ( rc = postSysCall( dataPtr, errno, -1, exp_errno ) ) != 0 ) {
      printf1( "ERROR: post-syscall setup failed (%d)\n", rc );
      goto EXIT;
    }
  
  EXIT_CLEANUP:
  EXIT:
    return rc;

  } else {

    if( ( rc = setup_ioctl_test( dataPtr ) ) ) {
      printf1( "Error setting up for ioctl test\n" );
      return rc;
    }
    
    printf( "Now testing ioctl SIOCDELTUNNEL\n" );
    if( rc = runIoctlTest( dataPtr, fd, SIOCDELTUNNEL, &ifr, sizeof( struct ifreq ), ENODEV ) != 0 ) {
      printf1( "Error running ioctl test\n" );
      rc = -1;
      goto EXIT_CLEANUP2;
    }
    
  EXIT_CLEANUP2:
    cleanup_ioctl_test();
    
    return rc;
    
  }
  
}

/* Use ``ip'' command to call indirectly in success case */
int test_ioctl_SIOCADDTUNNEL( laus_data* dataPtr ) {

  int rc = 0;
  int exp_errno = EFAULT;
  char socket_id[25];
  int request = SIOCADDTUNNEL;

  if( dataPtr->successCase ) {
  
    if( ( rc = auditArgNil( dataPtr ) ) != 0 ) {
      printf1( "Error setting up audit argument buffer\n" );
      return rc;
    }

   
    // Do pre-system call work
    if( (rc = preSysCall( dataPtr ) ) != 0 ) {
      printf1("ERROR: pre-syscall setup failed (%d)\n", rc);
      return rc;
    }
   
    // Execute system call
    RUNCOMMANDORDIE( "/sbin/ip tunnel add tunl42 mode ipip remote 192.168.43.43" );
   
    // Do post-system call work
    if( ( rc = postSysCall( dataPtr, errno, -1, exp_errno ) ) != 0 ) {
      printf1( "ERROR: post-syscall setup failed (%d)\n", rc );
      goto EXIT_CLEANUP;
    }
  
  EXIT_CLEANUP:
    RUNCOMMAND( "/sbin/ip tunnel del tunl42" );
  EXIT:
    return rc;

  } else {

    if( ( rc = setup_ioctl_test( dataPtr ) ) ) {
      printf1( "Error setting up for ioctl test\n" );
      return rc;
    }
    
    printf( "Now testing ioctl SIOCADDTUNNEL\n" );
    if( rc = runIoctlTest( dataPtr, fd, SIOCADDTUNNEL, &ifr, sizeof( struct ifreq ), ENODEV ) != 0 ) {
      printf1( "Error running ioctl test\n" );
      rc = -1;
      goto EXIT_CLEANUP2;
    }
    
  EXIT_CLEANUP2:
    cleanup_ioctl_test();
    
    return rc;
    
  }
  
}

/* Error case only: errno 22 */
/* MH: Need userland utility that invokes this */
int test_ioctl_SIOCSIFPFLAGS( laus_data* dataPtr ) {

  int rc = 0;

  if( dataPtr->successCase ) {
      rc = SKIP_TEST_CASE;
      goto EXIT;
  }

  if( ( rc = setup_ioctl_test( dataPtr ) ) ) {
    printf1( "Error setting up for ioctl test\n" );
    return rc;
  }

  printf( "Now testing ioctl SIOCGIFPFLAGS\n" );
  if( rc = runIoctlTest( dataPtr, fd, SIOCGIFPFLAGS, &ifr, sizeof( struct ifreq ), EINVAL ) != 0 ) {
    printf1( "Error running ioctl test\n" );
    rc = -1;
    goto EXIT_CLEANUP;
  }

  printf( "Now testing ioctl SIOCSIFPFLAGS\n" );
  if( rc = runIoctlTest( dataPtr, fd, SIOCSIFPFLAGS, &ifr, sizeof( struct ifreq ), EINVAL ) != 0 ) {
    printf1( "Error running ioctl test\n" );
    rc = -1;
    goto EXIT_CLEANUP;
  }

 EXIT_CLEANUP:
  cleanup_ioctl_test();

 EXIT:
  return rc;

}

/* Error case only: EINVAL */
/* set iface channel */
/* Obsolete?  See drivers/audit/syscall.c */
int test_ioctl_SIOCSIFLINK( laus_data* dataPtr ) {

  int rc = 0;

  if( dataPtr->successCase ) {
      rc = SKIP_TEST_CASE;
      goto EXIT;
  }

  if( ( rc = setup_ioctl_test( dataPtr ) ) ) {
    printf1( "Error setting up for ioctl test\n" );
    return rc;
  }

  /* TODO: Set up struct
   */

  printf( "Now testing ioctl SIOCSIFLINK\n" );
  if( rc = runIoctlTest( dataPtr, fd, SIOCSIFLINK, &ifr, 0, EINVAL ) != 0 ) { // 0 length, according to 
    printf1( "Error running ioctl test\n" );
    rc = -1;
    goto EXIT_CLEANUP;
  }

 EXIT_CLEANUP:
  cleanup_ioctl_test();

 EXIT:
  return rc;

}

/* Error only at the moment: EOPNOTSUPP */
/* MH: Need to find working example of SIOCETHTOOL */
/* MH: Need userland utility that invokes this */
int test_ioctl_SIOCETHTOOL( laus_data* dataPtr ) {

  int rc = 0;
  //  struct ethtool_cmd ec;

  if( dataPtr->successCase ) {
      rc = SKIP_TEST_CASE;
      goto EXIT;
  }

  goto EXIT;

  /*  ec.cmd = ETHTOOL_GDRVINFO;

  if( ( rc = setup_ioctl_test( dataPtr ) ) ) {
    printf1( "Error setting up for ioctl test\n" );
    return rc;
  }

  printf( "Now testing ioctl SIOCETHTOOL\n" );
  if( rc = runIoctlTest( dataPtr, fd, SIOCETHTOOL, &ifr, sizeof( struct ifreq ), ENODEV ) != 0 ) {
    printf1( "Error running ioctl test\n" );
    rc = -1;
    goto EXIT_CLEANUP;
  }
  */

 EXIT_CLEANUP:
  cleanup_ioctl_test();

 EXIT:
  return rc;

}

/* Tested and working */
int test_ioctl_SIOCSIFFLAGS( laus_data* dataPtr ) {

  int rc = 0;

  if( ( rc = setup_ioctl_test( dataPtr ) ) ) {
    printf1( "Error setting up for ioctl test\n" );
    return rc;
  }

  printf( "Now testing ioctl SIOCGIFFLAGS\n" );
  if( rc = runIoctlTest( dataPtr, fd, SIOCGIFFLAGS, &ifr, sizeof( struct ifreq ), ENODEV ) != 0 ) {
    printf1( "Error running ioctl test\n" );
    rc = -1;
    goto EXIT_CLEANUP;
  }

  printf( "Now testing ioctl SIOCSIFFLAGS\n" );
  if( rc = runIoctlTest( dataPtr, fd, SIOCSIFFLAGS, &ifr, sizeof( struct ifreq ), ENODEV ) != 0 ) {
    printf1( "Error running ioctl test\n" );
    rc = -1;
    goto EXIT_CLEANUP;
  }

 EXIT_CLEANUP:
  cleanup_ioctl_test();

 EXIT:
  return rc;

}

/* Tested and working */
int test_ioctl_SIOCSIFADDR( laus_data* dataPtr ) {

  int rc = 0;

  if( ( rc = setup_ioctl_test( dataPtr ) ) ) {
    printf1( "Error setting up for ioctl test\n" );
    return rc;
  }

  printf( "Now testing ioctl SIOCGIFADDR\n" );
  if( rc = runIoctlTest( dataPtr, fd, SIOCGIFADDR, &ifr, sizeof( struct ifreq ), ENODEV ) != 0 ) {
    printf1( "Error running ioctl test\n" );
    rc = -1;
    goto EXIT_CLEANUP;
  }

  printf( "Now testing ioctl SIOCSIFADDR\n" );
  if( rc = runIoctlTest( dataPtr, fd, SIOCSIFADDR, &ifr, sizeof( struct ifreq ), EINVAL ) != 0 ) {
    printf1( "Error running ioctl test\n" );
    rc = -1;
    goto EXIT_CLEANUP;
  }

 EXIT_CLEANUP:
  cleanup_ioctl_test();

  return rc;

}

int test_ioctl_SIOCSIFDSTADDR( laus_data* dataPtr ) {

  int rc = 0;

  if( ( rc = setup_ioctl_test( dataPtr ) ) ) {
    printf1( "Error setting up for ioctl test\n" );
    return rc;
  }

  printf( "Now testing ioctl SIOCGIFDSTADDR\n" );
  if( rc = runIoctlTest( dataPtr, fd, SIOCGIFDSTADDR, &ifr, sizeof( struct ifreq ), ENODEV ) != 0 ) {
    printf1( "Error running ioctl test\n" );
    goto EXIT_CLEANUP;
  }

  printf( "Now testing ioctl SIOCSIFDSTADDR\n" );
  if( rc = runIoctlTest( dataPtr, fd, SIOCSIFDSTADDR, &ifr, sizeof( struct ifreq ), EINVAL ) != 0 ) {
    printf1( "Error running ioctl test\n" );
    goto EXIT_CLEANUP;
  }

 EXIT_CLEANUP:
  cleanup_ioctl_test();

  return rc;

}

int test_ioctl_SIOCSIFBRDADDR( laus_data* dataPtr ) {

  int rc = 0;

  if( ( rc = setup_ioctl_test( dataPtr ) ) ) {
    printf1( "Error setting up for ioctl test\n" );
    return rc;
  }

  printf( "Now testing ioctl SIOCGIFBRDADDR\n" );
  if( rc = runIoctlTest( dataPtr, fd, SIOCGIFBRDADDR, &ifr, sizeof( struct ifreq ), ENODEV ) != 0 ) {
    printf1( "Error running ioctl test\n" );
    goto EXIT_CLEANUP;
  }

  printf( "Now testing ioctl SIOCSIFBRDADDR\n" );
  if( rc = runIoctlTest( dataPtr, fd, SIOCSIFBRDADDR, &ifr, sizeof( struct ifreq ), EINVAL ) != 0 ) {
    printf1( "Error running ioctl test\n" );
    goto EXIT_CLEANUP;
  }

 EXIT_CLEANUP:
  cleanup_ioctl_test();

  return rc;

}

int test_ioctl_SIOCSIFNETMASK( laus_data* dataPtr ) {

  int rc = 0;

  if( ( rc = setup_ioctl_test( dataPtr ) ) ) {
    printf1( "Error setting up for ioctl test\n" );
    return rc;
  }

  printf( "Now testing ioctl SIOCGIFNETMASK\n" );
  if( rc = runIoctlTest( dataPtr, fd, SIOCGIFNETMASK, &ifr, sizeof( struct ifreq ), ENODEV ) != 0 ) {
    printf1( "Error running ioctl test\n" );
    goto EXIT_CLEANUP;
  }

  printf( "Now testing ioctl SIOCSIFNETMASK\n" );
  if( rc = runIoctlTest( dataPtr, fd, SIOCSIFNETMASK, &ifr, sizeof( struct ifreq ), EINVAL ) != 0 ) {
    printf1( "Error running ioctl test\n" );
    goto EXIT_CLEANUP;
  }

 EXIT_CLEANUP:
  cleanup_ioctl_test();

  return rc;

}

int test_ioctl_SIOCSIFMTU( laus_data* dataPtr ) {

  int rc = 0;

  if( ( rc = setup_ioctl_test( dataPtr ) ) ) {
    printf1( "Error setting up for ioctl test\n" );
    return rc;
  }

  printf( "Now testing ioctl SIOCGIFMTU\n" );
  if( rc = runIoctlTest( dataPtr, fd, SIOCGIFMTU, &ifr, sizeof( struct ifreq ), ENODEV ) != 0 ) {
    printf1( "Error running ioctl test\n" );
    goto EXIT_CLEANUP;
  }

  printf( "Now testing ioctl SIOCSIFMTU\n" );
  if( rc = runIoctlTest( dataPtr, fd, SIOCSIFMTU, &ifr, sizeof( struct ifreq ), ENODEV ) != 0 ) {
    printf1( "Error running ioctl test\n" );
    goto EXIT_CLEANUP;
  }

 EXIT_CLEANUP:
  cleanup_ioctl_test();

  return rc;

}

int test_ioctl_SIOCSIFHWADDR( laus_data* dataPtr ) {

  int rc = 0;

  if( ( rc = setup_ioctl_test( dataPtr ) ) ) {
    printf1( "Error setting up for ioctl test\n" );
    return rc;
  }

  // eth0 only (?) TODO: figure out how to automate this test
  if( strcmp( ifname, "eth0" ) == 0 ) {
    printf( "Now testing ioctl SIOCGIFHWADDR\n" );
    if( rc = runIoctlTest( dataPtr, fd, SIOCGIFHWADDR, &ifr, sizeof( struct ifreq ), EINVAL ) != 0 ) {
      printf1( "Error running ioctl test\n" );
      goto EXIT_CLEANUP;
    }
  }

  if( strcmp( ifname, "eth0" ) == 0 ) {
    printf( "Now testing ioctl SIOCSIFHWADDR\n" );
    if( rc = runIoctlTest( dataPtr, fd, SIOCSIFHWADDR, &ifr, sizeof( struct ifreq ), EINVAL ) != 0 ) {
      printf1( "Error running ioctl test\n" );
      goto EXIT_CLEANUP;
    }
  }

 EXIT_CLEANUP:
  cleanup_ioctl_test();

  return rc;

}

int test_ioctl_SIOCSIFTXQLEN( laus_data* dataPtr ) {

  int rc = 0;

  if( ( rc = setup_ioctl_test( dataPtr ) ) ) {
    printf1( "Error setting up for ioctl test\n" );
    return rc;
  }

  printf( "Now testing ioctl SIOCGIFTXQLEN\n" );
  if( rc = runIoctlTest( dataPtr, fd, SIOCGIFTXQLEN, &ifr, sizeof( struct ifreq ), ENODEV ) != 0 ) {
    printf1( "Error running ioctl test\n" );
    goto EXIT_CLEANUP;
  }

  printf( "Now testing ioctl SIOCSIFTXQLEN\n" );
  if( rc = runIoctlTest( dataPtr, fd, SIOCSIFTXQLEN, &ifr, sizeof( struct ifreq ), ENODEV ) != 0 ) {
    printf1( "Error running ioctl test\n" );
    goto EXIT_CLEANUP;
  }

 EXIT_CLEANUP:
  cleanup_ioctl_test();

  return rc;

}

int test_ioctl_SIOCSIFHWBROADCAST( laus_data* dataPtr ) {

  int rc = 0;

  if( ( rc = setup_ioctl_test( dataPtr ) ) ) {
    printf1( "Error setting up for ioctl test\n" );
    return rc;
  }

  hw_set(&ifr.ifr_hwaddr, "\xa5\xa5\xa5\xa5\xa5\xa5", 6);
  ifr.ifr_hwaddr.sa_family = ARPHRD_ETHER;
  printf( "Now testing ioctl SIOCSIFHWBROADCAST\n" );
  if( rc = runIoctlTest( dataPtr, fd, SIOCSIFHWBROADCAST, &ifr, sizeof( struct ifreq ), ENODEV ) != 0 ) {
    printf1( "Error running ioctl test\n" );
    goto EXIT_CLEANUP;
  }

 EXIT_CLEANUP:
  cleanup_ioctl_test();

  return rc;

}

int test_ioctl_SIOCSARP( laus_data* dataPtr ) {

  int rc = 0;

  if( ( rc = setup_ioctl_test( dataPtr ) ) ) {
    printf1( "Error setting up for ioctl test\n" );
    return rc;
  }

  hw_set(&ifr.ifr_hwaddr, "\xa5\xa5\xa5\xa5\xa5\xa5", 6);
  ifr.ifr_hwaddr.sa_family = ARPHRD_ETHER;
  in_set(&arp.arp_pa, addr.s_addr);
  hw_set(&arp.arp_ha, "\xa5\xa5\xa5\xa5\xa5\xa5", 6);
  /* arp.arp_ha.sa_family = ARPHRD_ETHER; */
  arp.arp_flags = ATF_PERM|ATF_PUBL|ATF_COM;
  strcpy(arp.arp_dev, ifname);

  printf( "Now testing ioctl SIOCSARP\n" );
  if( rc = runIoctlTest( dataPtr, fd, SIOCSARP, &arp, sizeof( struct arpreq ), ENODEV ) != 0 ) {
    printf1( "Error running ioctl test\n" );
    goto EXIT_CLEANUP;
  }

 EXIT_CLEANUP:
  cleanup_ioctl_test();

  return rc;

}

int test_ioctl_SIOCDARP( laus_data* dataPtr ) {

  int rc = 0;

  if( ( rc = setup_ioctl_test( dataPtr ) ) ) {
    printf1( "Error setting up for ioctl test\n" );
    return rc;
  }

  hw_set(&ifr.ifr_hwaddr, "\xa5\xa5\xa5\xa5\xa5\xa5", 6);
  ifr.ifr_hwaddr.sa_family = ARPHRD_ETHER;
  in_set(&arp.arp_pa, addr.s_addr);
  hw_set(&arp.arp_ha, "\xa5\xa5\xa5\xa5\xa5\xa5", 6);
  /* arp.arp_ha.sa_family = ARPHRD_ETHER; */
  arp.arp_flags = ATF_PERM|ATF_PUBL|ATF_COM;
  strcpy(arp.arp_dev, ifname);
  printf( "Now testing ioctl SIOCDARP\n" );
  if( rc = runIoctlTest( dataPtr, fd, SIOCDARP, &arp, sizeof( struct arpreq ), ENODEV ) != 0 ) {
    printf1( "Error running ioctl test\n" );
    goto EXIT_CLEANUP;
  }

 EXIT_CLEANUP:
  cleanup_ioctl_test();

  return rc;

}

int test_ioctl_SIOCADDRT( laus_data* dataPtr ) {

  int rc = 0;

  if( ( rc = setup_ioctl_test( dataPtr ) ) ) {
    printf1( "Error setting up for ioctl test\n" );
    return rc;
  }

  hw_set(&ifr.ifr_hwaddr, "\xa5\xa5\xa5\xa5\xa5\xa5", 6);
  ifr.ifr_hwaddr.sa_family = ARPHRD_ETHER;
  in_set(&arp.arp_pa, addr.s_addr);
  hw_set(&arp.arp_ha, "\xa5\xa5\xa5\xa5\xa5\xa5", 6);
  /* arp.arp_ha.sa_family = ARPHRD_ETHER; */
  arp.arp_flags = ATF_PERM|ATF_PUBL|ATF_COM;
  strcpy(arp.arp_dev, ifname);
  memset(&rt, 0, sizeof(rt));
  in_set(&rt.rt_dst, htonl(0x0afe0101));
  in_set(&rt.rt_gateway,
	 (addr.s_addr & htonl(0xffffff00)) | htonl(0xfe));
  rt.rt_flags = RTF_UP|RTF_GATEWAY|RTF_HOST;

  printf( "Now testing ioctl SIOCADDRT\n" );
  if( rc = runIoctlTest( dataPtr, fd, SIOCADDRT, 
		    ( dataPtr->successCase ? &rt : NULL ), 
		    ( dataPtr->successCase ? sizeof( struct rtentry ) : 0 ), 
		    EFAULT ) != 0 ) {
    printf1( "Error running ioctl test\n" );
    goto EXIT_CLEANUP;
  }

 EXIT_CLEANUP:
  cleanup_ioctl_test();

  return rc;

}

int test_ioctl_SIOCDELRT( laus_data* dataPtr ) {

  int rc = 0;

  if( ( rc = setup_ioctl_test( dataPtr ) ) ) {
    printf1( "Error setting up for ioctl test\n" );
    return rc;
  }

  hw_set(&ifr.ifr_hwaddr, "\xa5\xa5\xa5\xa5\xa5\xa5", 6);
  ifr.ifr_hwaddr.sa_family = ARPHRD_ETHER;
  in_set(&arp.arp_pa, addr.s_addr);
  hw_set(&arp.arp_ha, "\xa5\xa5\xa5\xa5\xa5\xa5", 6);
  /* arp.arp_ha.sa_family = ARPHRD_ETHER; */
  arp.arp_flags = ATF_PERM|ATF_PUBL|ATF_COM;
  strcpy(arp.arp_dev, ifname);
  memset(&rt, 0, sizeof(rt));
  in_set(&rt.rt_dst, htonl(0x0afe0101));
  in_set(&rt.rt_gateway,
	 (addr.s_addr & htonl(0xffffff00)) | htonl(0xfe));
  rt.rt_flags = RTF_UP|RTF_GATEWAY|RTF_HOST;

  printf( "Now testing ioctl SIOCADDRT\n" );
  if( rc = runIoctlTest( dataPtr, fd, SIOCADDRT, 
		    ( dataPtr->successCase ? &rt : NULL ), 
		    ( dataPtr->successCase ? sizeof( struct rtentry ) : 0 ), 
		    EFAULT ) != 0 ) {
    printf1( "Error running ioctl test\n" );
    goto EXIT_CLEANUP;
  }

  hw_set(&ifr.ifr_hwaddr, "\xa5\xa5\xa5\xa5\xa5\xa5", 6);
  ifr.ifr_hwaddr.sa_family = ARPHRD_ETHER;
  in_set(&arp.arp_pa, addr.s_addr);
  hw_set(&arp.arp_ha, "\xa5\xa5\xa5\xa5\xa5\xa5", 6);
  /* arp.arp_ha.sa_family = ARPHRD_ETHER; */
  arp.arp_flags = ATF_PERM|ATF_PUBL|ATF_COM;
  strcpy(arp.arp_dev, ifname);
  memset(&rt, 0, sizeof(rt));
  in_set(&rt.rt_dst, htonl(0x0afe0101));
  in_set(&rt.rt_gateway,
	 (addr.s_addr & htonl(0xffffff00)) | htonl(0xfe));
  rt.rt_flags = RTF_UP|RTF_GATEWAY|RTF_HOST;
  printf( "Now testing ioctl SIOCDELRT\n" );
  if( rc = runIoctlTest( dataPtr, fd, SIOCDELRT, &rt, sizeof( struct rtentry ), ESRCH ) != 0 ) {
    printf1( "Error running ioctl test\n" );
    goto EXIT_CLEANUP;
  }

 EXIT_CLEANUP:
  cleanup_ioctl_test();

  return rc;

}

int test_ioctl_SIOCSIFNAME( laus_data* dataPtr ) {

  int rc = 0;

  if( ( rc = setup_ioctl_test( dataPtr ) ) ) {
    printf1( "Error setting up for ioctl test\n" );
    return rc;
  }

  hw_set(&ifr.ifr_hwaddr, "\xa5\xa5\xa5\xa5\xa5\xa5", 6);
  ifr.ifr_hwaddr.sa_family = ARPHRD_ETHER;
  in_set(&arp.arp_pa, addr.s_addr);
  hw_set(&arp.arp_ha, "\xa5\xa5\xa5\xa5\xa5\xa5", 6);
  /* arp.arp_ha.sa_family = ARPHRD_ETHER; */
  arp.arp_flags = ATF_PERM|ATF_PUBL|ATF_COM;
  strcpy(arp.arp_dev, ifname);
  memset(&rt, 0, sizeof(rt));
  in_set(&rt.rt_dst, htonl(0x0afe0101));
  in_set(&rt.rt_gateway,
	 (addr.s_addr & htonl(0xffffff00)) | htonl(0xfe));
  rt.rt_flags = RTF_UP|RTF_GATEWAY|RTF_HOST;
  //must bring it down first
  ifr.ifr_flags = 0;

  if( rc = runIoctlTest( dataPtr, fd, SIOCSIFFLAGS, &ifr, sizeof( struct ifreq ), ENODEV ) != 0 ) {
    printf1( "Error running ioctl test\n" );
    goto EXIT_CLEANUP;
  }

  strcpy(ifr.ifr_newname, "foo0");

  if( rc = runIoctlTest( dataPtr, fd, SIOCSIFNAME, &ifr, sizeof( struct ifreq ), ENODEV ) != 0 ) {
    printf1( "Error running ioctl test\n" );
    goto EXIT_CLEANUP;
  }

  strcpy(ifr.ifr_name, "foo0");
  strcpy(ifr.ifr_newname, ifname);

  printf( "Now testing ioctl SIOCSIFNAME\n" );
  if( rc = runIoctlTest( dataPtr, fd, SIOCSIFNAME, &ifr, sizeof( struct ifreq ), ENODEV ) != 0 ) {
    printf1( "Error running ioctl test\n" );
    goto EXIT_CLEANUP;
  }



 EXIT_CLEANUP:
  cleanup_ioctl_test();

  return rc;

}

int test_ioctl_SIOCSIFMEM( laus_data* dataPtr ) {

  int rc = 0;

  if( dataPtr->successCase ) {
      rc = SKIP_TEST_CASE;
      goto EXIT;
  }

  if( ( rc = setup_ioctl_test( dataPtr ) ) ) {
    printf1( "Error setting up for ioctl test\n" );
    return rc;
  }

  if( !dataPtr->successCase ) {
    printf( "Now testing ioctl SIOCSIFMEM\n" );
    if( rc = runIoctlTest( dataPtr, fd, SIOCSIFMEM, &ifr, sizeof( struct ifreq ), EINVAL ) != 0 ) {
      printf1( "Error running ioctl test\n" );
      goto EXIT_CLEANUP;
    }
  } 

 EXIT_CLEANUP:
  cleanup_ioctl_test();

 EXIT:
  return rc;

}

int test_ioctl_SIOCSIFSLAVE( laus_data* dataPtr ) {

  int rc = 0;

  if( dataPtr->successCase ) {
      rc = SKIP_TEST_CASE;
      goto EXIT;
  }

  if( ( rc = setup_ioctl_test( dataPtr ) ) ) {
    printf1( "Error setting up for ioctl test\n" );
    return rc;
  }

  if( !dataPtr->successCase ) {
    printf( "Now testing ioctl SIOCSIFSLAVE\n" );
    if( rc = runIoctlTest( dataPtr, fd, SIOCSIFSLAVE, &ifr, sizeof( struct ifreq ), ENODEV ) != 0 ) {
      printf1( "Error running ioctl test\n" );
      goto EXIT_CLEANUP;
    }
  } 

 EXIT_CLEANUP:
  cleanup_ioctl_test();

 EXIT:
  return rc;

}

int test_ioctl_SIOCADDMULTI( laus_data* dataPtr ) {

  int rc = 0;

  if( dataPtr->successCase ) {
      rc = SKIP_TEST_CASE;
      goto EXIT;
  }

  if( ( rc = setup_ioctl_test( dataPtr ) ) ) {
    printf1( "Error setting up for ioctl test\n" );
    return rc;
  }

  if( !dataPtr->successCase ) {
    printf( "Now testing ioctl SIOCADDMULTI\n" );
    if( rc = runIoctlTest( dataPtr, fd, SIOCADDMULTI, &ifr, sizeof( struct ifreq ), ENODEV ) != 0 ) {
      printf1( "Error running ioctl test\n" );
      goto EXIT_CLEANUP;
    }
  } 

 EXIT_CLEANUP:
  cleanup_ioctl_test();

 EXIT:
  return rc;

}

int test_ioctl_SIOCDELMULTI( laus_data* dataPtr ) {

  int rc = 0;

  if ( dataPtr->successCase ) {
      rc = SKIP_TEST_CASE;
      goto EXIT;
  }

  if( ( rc = setup_ioctl_test( dataPtr ) ) ) {
    printf1( "Error setting up for ioctl test\n" );
    return rc;
  }

  if( !dataPtr->successCase ) {
    printf( "Now testing ioctl SIOCDELMULTI\n" );
    if( rc = runIoctlTest( dataPtr, fd, SIOCDELMULTI, &ifr, sizeof( struct ifreq ), ENODEV ) != 0 ) {
      printf1( "Error running ioctl test\n" );
      goto EXIT_CLEANUP;
    }
  } 

 EXIT_CLEANUP:
  cleanup_ioctl_test();

 EXIT:
  return rc;

}

int test_ioctl_SIOCSIFMETRIC( laus_data* dataPtr ) {

  int rc = 0;
  
  if ( dataPtr->successCase ) {
      rc = SKIP_TEST_CASE;
      goto EXIT;
  }

  if( ( rc = setup_ioctl_test( dataPtr ) ) ) {
    printf1( "Error setting up for ioctl test\n" );
    return rc;
  }

  if( !dataPtr->successCase ) {
    printf( "Now testing ioctl SIOCSIFMETRIC\n" );
    if( rc = runIoctlTest( dataPtr, fd, SIOCSIFMETRIC, &ifr, sizeof( struct ifreq ), ENODEV ) != 0 ) {
      printf1( "Error running ioctl test\n" );
      goto EXIT_CLEANUP;
    }
  } 

 EXIT_CLEANUP:
  cleanup_ioctl_test();

 EXIT:
  return rc;

}

int test_ioctl_SIOCSIFMAP( laus_data* dataPtr ) {

  int rc = 0;
  
  if( dataPtr->successCase ) {
      rc = SKIP_TEST_CASE;
      goto EXIT;
  }

  if( ( rc = setup_ioctl_test( dataPtr ) ) ) {
    printf1( "Error setting up for ioctl test\n" );
    return rc;
  }

  if( !dataPtr->successCase ) {
    printf( "Now testing ioctl SIOCSIFMAP\n" );
    if( rc = runIoctlTest( dataPtr, fd, SIOCSIFMAP, &ifr, sizeof( struct ifreq ), ENODEV ) != 0 ) {
      printf1( "Error running ioctl test\n" );
      goto EXIT_CLEANUP;
    }
  } 

 EXIT_CLEANUP:
  cleanup_ioctl_test();

 EXIT:
  return rc;

}

int test_ioctl_SIOCSIFBR( laus_data* dataPtr ) {

  int rc = 0;

  if( dataPtr->successCase ) {
      rc = SKIP_TEST_CASE;
      goto EXIT;
  }

  if( ( rc = setup_ioctl_test( dataPtr ) ) ) {
    printf1( "Error setting up for ioctl test\n" );
    return rc;
  }

  if( !dataPtr->successCase ) {
    printf( "Now testing ioctl SIOCSIFBR\n" );
    if( rc = runIoctlTest( dataPtr, fd, SIOCSIFBR, &ifr, 3 * sizeof( long ), EOPNOTSUPP ) != 0 ) {
      printf1( "Error running ioctl test\n" );
      goto EXIT_CLEANUP;
    } 
  }

 EXIT_CLEANUP:
  cleanup_ioctl_test();

 EXIT:
  return rc;

}

int test_ioctl_SIOCSIFENCAP( laus_data* dataPtr ) {

  int rc = 0;

  if( ( rc = setup_ioctl_test( dataPtr ) ) ) {
    printf1( "Error setting up for ioctl test\n" );
    return rc;
  }

  // Adapted from rrouted.c
  {
    int i, j, s;

    target.s_addr = htonl( INADDR_ANY );
    mask.s_addr = htonl( INADDR_ANY );
    dummynet.s_addr = htonl( 0x0affff00 );

    if( ( s = openpty( &slipfd ) ) < 0 ) {
      printf1( "Error openning PTY; rc = [%d]\n", s );
      goto EXIT_CLEANUP;
    }

    if( dataPtr->successCase ) {
      i = N_SLIP;
      if( ( j = ioctl( s, TIOCSETD, &i ) ) < 0 ) {
	printf1( "Error running ioctl TIOCSETD; rc = [%d], error = [%d]\n", j, errno );
	goto EXIT_CLEANUP;
      }
    }

    i=0;
    printf( "Now testing ioctl SIOCSIFENCAP\n" );
    if( rc = runIoctlTest( dataPtr, s, SIOCSIFENCAP, &i, sizeof( int ), EINVAL ) != 0 ) {
      printf1( "Error running ioctl test\n" );
      goto EXIT_CLEANUP;
    } 
  }

 EXIT_CLEANUP:
  cleanup_ioctl_test();

  return rc;

}
