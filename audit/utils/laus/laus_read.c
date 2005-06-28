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
**  FILE   : audit_read.c
**
**  PURPOSE: This file provides an interface to retrieve audit
**           audit records to aid the audit_verify function.
**
**
**  HISTORY:
**    07/03 originated by Jerone Young <jyoung5@us.ibm.com>
**    07/03 originated by <okir@suse.de>
**
**********************************************************************/

#include "includes.h"
#include <sys/mman.h>

#include "audit_read.h"

/* initize varibles in an aud_log structure */
void 
initialize_aud_log (struct aud_log * log)
{
	
	log->fd=0;
	log->p=(caddr_t)0;
	log->start=(caddr_t)0;
	log->end=(caddr_t)0;
	log->st_size=(size_t)0;
	log->filename="";
}

/* Open audit log */
int
open_audit_log (struct aud_log *audit_log, char *filename)
{
	struct stat stb;
	caddr_t data;
	int fd;
	struct laus_file_header * header;

	if ( audit_log->fd > 0){
		fprintf (stderr, "Audit log is already open");
		return -1;
	}	

	if ((fd = open (filename, O_RDONLY)) < 0 ){
		fprintf (stderr, "Unable to open: %s errno=%d %s\n",filename,errno,strerror(errno));
		return -1;
	}

	if (fstat(fd, &stb) < 0){
		fprintf(stderr, "Failed to stat: %s \n", filename);
		close (fd);
		return -2;
	}

	if (stb.st_size == 0) {
		fprintf(stderr, "Empty log file: %s\n", filename);
		close (fd);
		return -3;
	}


	data = (caddr_t) mmap(NULL, stb.st_size, PROT_READ, MAP_SHARED, fd, 0);
	if (data == NULL) {
		fprintf(stderr, "Failed to map: %s \n", filename);
		close (fd);	
		return -4;
	}
	header = (struct laus_file_header *) data;

        /*
	if (header->h_version != LAUS_VERSION) {
                fprintf(stderr, "Unable to read this audit file (file version %x)\n",
                                header->h_version);
                return -5;
        }
		
        if (header->h_msgversion != LAUS_VERSION) {
                fprintf(stderr, "Unable to read this audit file (kernel version %x)\n",
                                header->h_msgversion);
               	return -6;
        }
	*/
	
	/* map to struct */
	audit_log->fd 		= 	fd;
	audit_log->st_size	=	stb.st_size;
	audit_log->start	=	data + sizeof(struct laus_file_header);
	audit_log->end		=	header->h_count + audit_log->start;
	audit_log->p		=	audit_log->start;
	audit_log->filename	= 	filename;
	
	return 0;	
} 

/* Close audit log */
int 
close_audit_log (struct aud_log * audit_log)
{
	int result=0;	

	if (audit_log->fd > 0){
		close(audit_log->fd);
		audit_log->fd = 0;
		if (audit_log->start != NULL){
			munmap(audit_log->start - sizeof(struct laus_file_header), audit_log->st_size);
		}
		initialize_aud_log(audit_log); 
	}
	else if (audit_log->fd <= 0){
		fprintf(stderr, "File handle is already closed: %s\n", audit_log->filename);	
		result=-1;
	}
	return result;
}

/* Move to First Audit Record */
int rewind_audit_log (struct aud_log * audit_log) 
{
	if (audit_log->start && audit_log->p){
		audit_log->p=audit_log->start;
		return 0;
	}

	return -1;
}

/* Move to Next Audit Record */
struct aud_message_on_disk *
get_next_audit_record (struct aud_log * audit_log, struct laus_record_header_on_disk *hdr)
{
	//int i;
        struct aud_message_on_disk * msg;

	if (audit_log->fd <=0){
                fprintf (stderr, "Audit log passed into function has not been opened\n");
                return NULL;
        }

	if (audit_log->p + sizeof(*hdr) > audit_log->end) {
		return NULL;
	}
        memcpy(hdr, audit_log->p, sizeof(struct laus_record_header_on_disk));
	audit_log->p += sizeof(*hdr);
	msg = (struct aud_message_on_disk *) audit_log->p;

	if (audit_log->p + hdr->r_size > audit_log->end) {
printf(" >>> log pointer (%p) plus header rsize (%zx) greater than end of log (%p) <<<\n", audit_log->p, hdr->r_size, audit_log->end);
		return NULL;
	}

	audit_log->p += hdr->r_size;

	/* nul timestamp indicates end of records (this happens
        * with bin files) */
        if (hdr->r_time == 0) {
                return NULL;
	}

	return msg;
}

