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
**  FILE   : failsafe_config.c
**
**  PURPOSE: This file is used by the failsafe test to create and
**           configuration that is to be used for the failsafe test.
**
**  DESCRIPTION: This file contains various routines used to establish
**  the audit configuration.  The functions build into the laus_test
**  framework to provide a basis to verify that the Linux Audit System
**  performs as designed.
**
**  Each function will documents its execution.
**
**
**  HISTORY:
**    09/03 originated by Tom Lendacky (toml@us.ibm.com)
**
**********************************************************************/

#include "includes.h"

#include "failsafe.h"
#include "failsafe_config.h"


extern char notify_exe[PATH_MAX];
extern int semid;
extern int msgid;
extern int debug;

// Flag to indicate if initialization has taken place.
int init = 0;

// Pre-determined audit.conf contents
char audit_conf[] =
	"# kernel interface\n"
	"device-file = \"/dev/audit\";\n"
	"\n"
	"# filter config\n"
	"filter-config = \"/etc/audit/filter.conf\";\n"
	"\n"
	"# Standard output method is bin mode.\n"
	"#\n"
	"output {\n"
	"\tmode = bin;\n"
	"\tnum-files = 3;\n"
	"\tfile-size = %i;\n"
	"\tfile-name = \"/var/log/audit.d/bin\";\n"
	"\tnotify = \"%s --msgid %d --rc %d --semid %d --debug %d\";\n"
	"\tcurrent = \"/var/log/audit\";\n"
	"\tsync = yes;\n"
	"\terror {\n"
	"\t\taction {\n"
	"\t\t\ttype = suspend;\n"
	"\t\t};\n"
	"\t};\n"
	"};\n"
	"\n"
	"# Disk usage thresholds.\n"
	"# These thresholds are checked at regular intervals when\n"
	"# append mode is used.\n"
	"# (bin mode doesn't require these checks as the bin files\n"
	"# are preallocated).\n"
	"threshold disk-space-low {\n"
	"\tspace-left = 512;\n"
	"\taction {\n"
	"\t\ttype = syslog;\n"
	"\t\tfacility = security;\n"
	"\t\tpriority = warning;\n"
	"\t};\n"
	"\taction {\n"
	"\t\ttype = notify;\n"
	"\t\tcommand = \"/usr/local/bin/page-admin\";\n"
	"\t};\n"
	"};\n"
	"\n"
	"threshold disk-full {\n"
	"\tspace-left = 0;\n"
	"\taction {\n"
	"\t\ttype = syslog;\n"
	"\t\tfacility = security;\n"
	"\t\tpriority = crit;\n"
	"\t};\n"
	"};\n";

// Pre-determined filter.conf contents
char filter_conf[] =
	"# Failsafe test filters\n"
	"event user-message = always;\n";


/**********************************************************************
**  Function: init_audit_conf
**    Sets the audit configuration for the tests
**
**   1) Backs up the audit configuration files that will be changed.
**   2) Creates the new audit configuration files.
**   3) Creates new smaller size audit log files.
**
**********************************************************************/
int init_audit_conf(int notify_rc, int notify_semid) {

	FILE *file = NULL;


	if (init)
		return 0;

	init = 1;

	// Backup up the audit.conf and filter.conf files
	printf5("Calling backupFile for file '%s'\n", AUDIT_CONF);
	if (backupFile(AUDIT_CONF) < 0) {
		printf1("ERROR: backupFile failed for file '%s'\n", AUDIT_CONF);
		return -1;
	}
	printf5("Calling backupFile for file '%s'\n", FILTER_CONF);
	if (backupFile(FILTER_CONF) < 0) {
		printf1("ERROR: backupFile failed for file '%s'\n", FILTER_CONF);
		return -1;
	}

	// Open and write the new audit.conf file
	printf5("Writing new '%s' configuration\n", AUDIT_CONF);
	file = fopen(AUDIT_CONF, "w");
	if (!file) {
		printf1("ERROR: fopen failed for '%s' - errno=%d %s\n", AUDIT_CONF, errno, strerror(errno));
		return -1;
	}
	fprintf(file, audit_conf,
		AUDIT_FILE_SIZE,
		notify_exe,
		msgid,
		notify_rc,
		notify_semid,
		debug);
	fclose(file);

	// Open and write the new filter.conf file
	printf5("Writing new '%s' configuration\n", FILTER_CONF);
	file = fopen(FILTER_CONF, "w");
	if (!file) {
		printf1("ERROR: fopen failed for '%s' - errno=%d %s\n", FILTER_CONF, errno, strerror(errno));
		return -1;
	}
	fprintf(file, filter_conf);
	fclose(file);

	return 0;
}


/**********************************************************************
**  Function: term_audit_conf
**    Restores the original audit configuration
**
**   1) Deletes and restores the audit configuration files.
**   2) Restores the original size audit log files.
**
**********************************************************************/
int term_audit_conf(void) {

	if (!init)
		return 0;

	printf5("Calling restoreFile for file '%s'\n", AUDIT_CONF);
	restoreFile(AUDIT_CONF);
	printf5("Calling restoreFile for file '%s'\n", FILTER_CONF);
	restoreFile(FILTER_CONF);

	// In order to use original pre-allocated log files:
	//   - Stop auditd
	//   - Erase log files
	//   - Start auditd (now using original config files)
	stopClearStartAudit();

	init = 0;

	return 0;
}
