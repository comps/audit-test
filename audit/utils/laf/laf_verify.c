/* =======================================================================
 * Copyright (C) 2005 Hewlett-Packard Company
 * Written by Amy Griffis <amy.griffis@hp.com>
 * 
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License.
 * 
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 * 
 *   You should have received a copy of the GNU General Public License
 *   with this package; if not, write to the Free Software Foundation,
 *   Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 * 
 * ======================================================================= 
 */

/*
 * Return value:
 * -1  fatal error
 *  0+ number of records found
 */

#include "includes.h"
#include <regex.h>

/* constant based on FORMAT_MAX from audit user-space sources */
#define AUDIT_RECORD_MAX 8192

typedef struct parse_info {
    regex_t pbuf;	// regex pattern buffer
    regmatch_t *pmatch;	// list of match locations
    int nmatch;		// number of matches
    char *regex;	// regex string
    char *type;		// record type string
    int (*handler)(laus_data *, struct parse_info *, char *, char *); // match handler
} parse_info_t;

static int init_parse_info(u_int16_t, parse_info_t *);
static void free_parse_info(parse_info_t *);

static int match_type(laus_data *, parse_info_t *, char *, char *);
static int match_syscall_record(laus_data *, parse_info_t *, char *, char *);


int laf_verify_log(laus_data *dataPtr, log_options logOption) {

    int rc = 0;
    int records_found = 0;
    FILE *fp = NULL;	// audit log file stream
    char *buf = NULL;	// audit log line buffer
    parse_info_t tinfo;	// info for parsing record type field
    parse_info_t vinfo;	// info for parsing a record variety

    if ((fp = fopen("/var/log/audit/audit.log", "r")) == NULL) {
	printf1("ERROR: Unable to open audit log: %s\n", strerror(errno));
	rc = -1;
	goto exit_laf_verify_log;
    }

    if ((buf = (char *)malloc(AUDIT_RECORD_MAX)) == NULL) {
	printf1("ERROR: malloc: %s\n", strerror(errno));
	rc = -1;
	goto exit_laf_verify_log;
    }

    if ((init_parse_info(0, &tinfo) < 0) ||
	(init_parse_info(dataPtr->msg_type, &vinfo) < 0)) {
	printf1("ERROR: Unable to initialize regex info.\n");
	rc = -1;
	goto exit_laf_verify_log;
    }

    while ((fgets(buf, AUDIT_RECORD_MAX, fp)) != NULL) {

	/* determine record type field */
	if (tinfo.handler(dataPtr, &tinfo, buf, vinfo.type) <= 0) {
	    continue;
	}

	/* match rest of record */
	if ((vinfo.handler)(dataPtr, &vinfo, buf, NULL)) {
	    printf9("matched record: %s\n", buf);
	    records_found++;
	}
    }

    /* TODO check for messages that haven't arrived in log? */

    free_parse_info(&tinfo);
    free_parse_info(&vinfo);

    printf5("records found: %d\n", records_found);
    rc = records_found;

exit_laf_verify_log:
    if (buf) {
	free(buf);
    }
    if (fp) {
	fclose(fp);
    }

    return rc;
}

static int init_parse_info(u_int16_t type, parse_info_t *info) {
    int rc = 0;
    char *regex_type = "^type=([[:alnum:]]+)";
    char *regex_syscall =
	"(msg)=([^[:space:]]*)[[:space:]]+"
	"(arch)=([^[:space:]]*)[[:space:]]+"
	"(syscall)=([^[:space:]]*)[[:space:]]+"
	"(success)=([^[:space:]]*)[[:space:]]+"
	"(exit)=([^[:space:]]*)[[:space:]]+"
	"(a0)=([^[:space:]]*)[[:space:]]+"
	"(a1)=([^[:space:]]*)[[:space:]]+"
	"(a2)=([^[:space:]]*)[[:space:]]+"
	"(a3)=([^[:space:]]*)[[:space:]]+"
	"(items)=([^[:space:]]*)[[:space:]]+"
	"(pid)=([^[:space:]]*)[[:space:]]+"
	"(auid)=([^[:space:]]*)[[:space:]]+"
	"(uid)=([^[:space:]]*)[[:space:]]+"
	"(gid)=([^[:space:]]*)[[:space:]]+"
	"(euid)=([^[:space:]]*)[[:space:]]+"
	"(suid)=([^[:space:]]*)[[:space:]]+"
	"(fsuid)=([^[:space:]]*)[[:space:]]+"
	"(egid)=([^[:space:]]*)[[:space:]]+"
	"(sgid)=([^[:space:]]*)[[:space:]]+"
	"(fsgid)=([^[:space:]]*)[[:space:]]+"
	"(comm)=([^[:space:]]*)[[:space:]]+"
	"(exe)=([^[:space:]]*)";

    switch(type) {
	case 0: // "unknown" message type
	    info->type = NULL;
	    info->regex = strdup(regex_type);
	    info->handler = &match_type;
	    break;
	case AUDIT_MSG_SYSCALL:
	    info->type = strdup("SYSCALL");
	    info->regex = strdup(regex_syscall);
	    info->handler = &match_syscall_record;
	    break;
	default:
	    printf1("ERROR: Urecognized record type %d\n", type);
	    info->type = NULL;
	    info->regex = NULL;
	    info->handler = NULL;
	    rc = -1;
	    goto exit_init_parse_info;
    }

    if (regcomp(&(info->pbuf), info->regex, REG_EXTENDED) != 0) {
        printf1("ERROR: Unable to compile regex %s\n", info->regex);
	rc = -1;
	goto exit_init_parse_info;
    }

    info->nmatch = info->pbuf.re_nsub + 1; // account for match of entire expression
    if ((info->pmatch = malloc(sizeof(regmatch_t) * info->nmatch)) == NULL) {
        printf1("ERROR: malloc: %s\n", strerror(errno));
	rc = -1;
    }

exit_init_parse_info:
    return rc;
}

static void free_parse_info(parse_info_t *info) {
    regfree(&(info->pbuf));
    if (info->pmatch) {
	free(info->pmatch);
    }
    if (info->regex) {
	free(info->regex);
    }
    if (info->type) {
	free(info->type);
    }
}

static int match_type(laus_data *dataPtr, parse_info_t *info, char *buf, char *etype) {
    int rc = 0;		// -1 = error, 0 = no match, 1 = matched
    char *type = NULL;	// type string
    int typelen;	// type string length

    if (regexec(&(info->pbuf), buf, info->nmatch, info->pmatch, 0) == REG_NOMATCH) {
        printf2("WARNING: Urecognized record type: %s\n", buf);
	rc = -1;
	goto exit_match_type;
    }

    typelen = info->pmatch[info->nmatch-1].rm_eo -
	info->pmatch[info->nmatch-1].rm_so;
    if ((type = malloc(typelen)) == NULL) {
	printf1("ERROR: malloc: %s\n", strerror(errno));
	rc = -1;
	goto exit_match_type;
    }

    strncpy(type, buf+info->pmatch[info->nmatch-1].rm_so, typelen);
    type[typelen] = '\0';

    if ((strcmp(type, etype)) == 0) {
        rc = 1;
    }

exit_match_type:
    if (type) {
	free(type);
    }
    return rc;
}

static int match_syscall_record(laus_data *dataPtr, parse_info_t *info, char *buf, char *unused) {
    int rc = 1;		  // -1 = error, 0 = no match, 1 = matched
    int i;		  // loop index
    char *key = NULL;	  // key string
    char *value = NULL;	  // value string
    int keylen, valuelen; // key, value string lengths

    if (regexec(&(info->pbuf), buf, info->nmatch, info->pmatch, 0) == REG_NOMATCH) {
        printf2("WARNING: Urecognized SYSCALL record format: %s\n", buf);
	rc = -1;
	goto exit_match_syscall_record;
    }

    /* skip match for entire expression */
    for (i = 1; i < info->nmatch; i+=2) {

	/* copy key,value into temporary buffers */
	keylen = info->pmatch[i].rm_eo - info->pmatch[i].rm_so;
	if ((key = malloc(keylen)) == NULL) {
	    printf1("ERROR: malloc: %s\n", strerror(errno));
	    rc = -1;
	    goto exit_match_syscall_record;
	}

	strncpy(key, buf+info->pmatch[i].rm_so, keylen);
	key[keylen] = '\0';

	valuelen = info->pmatch[i+1].rm_eo - info->pmatch[i+1].rm_so;
	if ((value = malloc(valuelen)) == NULL) {
	    printf1("ERROR: malloc: %s\n", strerror(errno));
	    rc = -1;
	    goto exit_match_syscall_record;
	}

	strncpy(value, buf+info->pmatch[i+1].rm_so, valuelen);
	value[valuelen] = '\0';

	/* use the index into pmatch instead of a string comparison  to speed this up */
	if (strcmp(key, "arch") == 0) { // XXX dataPtr->msg_arch
	} else if (strcmp(key, "syscall") == 0) {
	    if (dataPtr->laus_var_data.syscallData.code != atoi(value)) {
		rc = 0;
		break;
	    }
	} else if (strcmp(key, "success") == 0) { // XXX ??
	} else if (strcmp(key, "exit") == 0) {
	    /*
	    if ((dataPtr->laus_var_data.syscallData.result != NO_RETURN_CODE) &&
		(dataPtr->laus_var_data.syscallData.result != atoi(value))) {
		rc = 0;
		break;
	    }
	    */
	} else if (strcmp(key, "a0") == 0) { // XXX dataPtr->laus_var_data.syscallData.data
	} else if (strcmp(key, "a1") == 0) {
	} else if (strcmp(key, "a2") == 0) {
	} else if (strcmp(key, "a3") == 0) {
	} else if (strcmp(key, "pid") == 0) {
	    if ((dataPtr->msg_pid != NO_PID_CHECK) &&
		(dataPtr->msg_pid != atoi(value))) {
		rc = 0;
		break;
	    }
	} else if (strcmp(key, "auid") == 0) {
	    if ((dataPtr->msg_login_uid != NO_ID_CHECK) &&
		(dataPtr->msg_login_uid != atoi(value))) {
 		rc = 0;
		break;
	    }
	} else if (strcmp(key, "uid") == 0) { // XXX ??
	} else if (strcmp(key, "gid") == 0) { // XXX ??
	} else if (strcmp(key, "euid") == 0) {
	    /*
	    if ((dataPtr->msg_euid != NO_ID_CHECK) && 
		(dataPtr->msg_euid != atoi(value))) {
		rc = 0;
		break;
	    }
	    */
	} else if (strcmp(key, "suid") == 0) {
	    /*
	    if ((dataPtr->msg_suid != NO_ID_CHECK) && 
		(dataPtr->msg_suid != atoi(value))) {
		rc = 0;
		break;
	    }
	    */
	} else if (strcmp(key, "fsuid") == 0) {
	    /*
	    if ((dataPtr->msg_fsuid != NO_ID_CHECK) && 
		(dataPtr->msg_fsuid != atoi(value))) {
		rc = 0;
		break;
	    }
	    */
	} else if (strcmp(key, "egid") == 0) {
	    /*
	    if ((dataPtr->msg_egid != NO_ID_CHECK) && 
		(dataPtr->msg_egid != atoi(value))) {
		matched = 0;
		break;
	    }
	    */
	} else if (strcmp(key, "sgid") == 0) {
	    /*
	    if ((dataPtr->msg_sgid != NO_ID_CHECK) && 
		(dataPtr->msg_sgid != atoi(value))) {
		matched = 0;
		break;
	    }
	    */
	} else if (strcmp(key, "fsgid") == 0) {
	    /*
	    if ((dataPtr->msg_fsgid != NO_ID_CHECK) && 
		(dataPtr->msg_fsgid != atoi(value))) {
		matched = 0;
		break;
	    }
	    */
	} else if (strcmp(key, "comm") == 0) { // XXX ??
	} else if (strcmp(key, "exe") == 0) { // XXX ??
	}

	free(key);
	free(value);
	key = NULL;
	value = NULL;
    }

exit_match_syscall_record:
    if (key) {
	free(key);
    }
    if (value) {
	free(value);
    }
    return rc;
}
