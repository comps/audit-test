/**********************************************************************
**   Copyright (C) International Business Machines  Corp., 2003
**   Copyright (C) SuSE Linux AG 2003
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
**  FILE   : audit_read.h
**
**  PURPOSE: This file defines the interface to get audit records.
**
**
**  HISTORY:
**    07/03 originated by Jerone Young <jyoung5@us.ibm.com>
**    07/03 originated by <okir@suse.de>
**
**********************************************************************/

#ifndef _AUDIT_READ_H
#define _AUDIT_READ_H

#include <asm/param.h>
#ifdef CONFIG_AUDIT_LAUS
#include <laus.h>
#include <linux/audit.h>
#else
#include "laus_info.h"
#endif
        
#if (defined(__S390X) || defined(__X86_64) || defined(__PPC64)) && defined(__MODE_32)
// This is needed for accurate processing of headers on 64bit platforms when test suite
//   is compiled in 31/32bit mode but running on a 64bit kernel (emulation).
//   auditd is running in 64bit mode but compilation of the test suite yields
//   data structures whose sizes are different.
struct laus_record_header_on_disk {
        __u64                   r_time;
        __u64                   r_size;
        /* Followed by kernel message */
};
struct aud_message_on_disk {
        uint32_t        msg_seqnr;
        uint16_t        msg_type;
        uint16_t        msg_arch;
        pid_t           msg_pid;
        __u64           msg_size;
        __u64           msg_timestamp;
        unsigned int    msg_audit_id;
        unsigned int    msg_login_uid;
        unsigned int    msg_euid, msg_ruid, msg_suid, msg_fsuid;
        unsigned int    msg_egid, msg_rgid, msg_sgid, msg_fsgid;
        /* Event name */
        char            msg_evname[AUD_MAX_EVNAME];
        unsigned char   msg_data[0];
};
#else
#define laus_record_header_on_disk 	laus_record_header
#define aud_message_on_disk 		aud_message
#endif

                                                                      
struct aud_log {
	int fd; 
	caddr_t p, start,end;
	size_t st_size;
	char * filename;
};


/* Function Prototypes */
void initialize_aud_log (struct aud_log * log);
int open_audit_log (struct aud_log *audit_log, char *filename);
int close_audit_log (struct aud_log * audit_log);
int rewind_audit_log (struct aud_log * audit_log);
struct aud_message_on_disk * get_next_audit_record (struct aud_log * audit_log, struct laus_record_header_on_disk *hdr);

#endif
