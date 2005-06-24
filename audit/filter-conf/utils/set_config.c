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
**  FILE   : set_config.c
**
**  PURPOSE: This file is used by the filter configuration test to
**           create and activate the filter configuration that is to
**           be tested.
**
**  DESCRIPTION: This file contains various routines used to establish
**  the filter configuration.  The functions build into the laus_test
**  framework to provide a basis to verify that the Linux Audit System
**  accurately logs the event based upon the filter configuration.
**
**  Each function will documents its execution.
**
**
**  HISTORY:
**    08/03 originated by Tom Lendacky (toml@us.ibm.com)
**
**********************************************************************/

#include "includes.h"

#include "filterconf.h"
#include "set_config.h"

#include <linux/kdev_t.h>


// Device name to be used by test(s)
extern char test_devname[DEVNAME_LEN];

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
	"\tnum-files = 4;\n"
	"\tfile-size = 20M;\n"
	"\tfile-name = \"/var/log/audit.d/bin\";\n"
	"\tnotify = \"/usr/sbin/audbin-notify\";\n"
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
	"\tspace-left = 10M;\n"
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

// Pre-determined filter.conf contents (predicate defs and sets)
//   (actual filter statement being tested is included via the
//    'include "filter-test.conf"' statement)
char filter_conf[] =
	"# Predicates and filters\n"
	"predicate is-null           = eq(0);\n"
	"predicate is-not-null       = ne(0);\n"
	"predicate is-negative       = lt(0);\n"
	"predicate is-not-positive   = le(0);\n"
	"predicate is-not-negative   = ge(0);\n"
	"predicate is-positive       = gt(0);\n"
	"\n"
	"predicate is-root           = eq(0);\n"
	"predicate is-not-root       = ne(0);\n"
	"predicate is-loginuid       = eq(%d);\n"	// fprintf with uid of supplied login user
	"predicate is-not-loginuid   = ne(%d);\n"	// fprintf with uid of supplied login user
	"predicate is-testuid        = eq(%d);\n"	// fprintf with uid of supplied user
	"predicate is-not-testuid    = ne(%d);\n"	// fprintf with uid of supplied user
	"predicate is-testgid        = eq(%d);\n"	// fprintf with gid of supplied user
	"predicate is-not-testgid    = ne(%d);\n"	// fprintf with gid of supplied user
	"\n"
	"# Numeric constants\n"
	"#   Used for integer comparison tests\n"
	"predicate eq-5              = eq(5);\n"
	"predicate eq-10             = eq(10);\n"
	"predicate eq-15             = eq(15);\n"
	"predicate ne-5              = ne(5);\n"
	"predicate ne-10             = ne(10);\n"
	"predicate ne-15             = ne(15);\n"
	"predicate lt-5              = lt(5);\n"
	"predicate lt-10             = lt(10);\n"
	"predicate lt-15             = lt(15);\n"
	"predicate le-5              = le(5);\n"
	"predicate le-10             = le(10);\n"
	"predicate le-15             = le(15);\n"
	"predicate ge-5              = ge(5);\n"
	"predicate ge-10             = ge(10);\n"
	"predicate ge-15             = ge(15);\n"
	"predicate gt-5              = gt(5);\n"
	"predicate gt-10             = gt(10);\n"
	"predicate gt-15             = gt(15);\n"
	"\n"
	"predicate mask-normal       = mask(__WSIGMASK, 0);\n"
	"predicate mask-abrtsignal   = mask(__WSIGMASK, __WSIGABRT);\n"
	"\n"
	"# String constants\n"
	"#   Used for string comparison tests\n"
	"predicate streq-file1       = streq(\""AUDIT_CONF"\");\n"
	"predicate streq-file2       = streq(\""FILTER_CONF"\");\n"
	"predicate prefix-path1      = prefix(\""STR_PATH1"\");\n"
	"predicate prefix-path2      = prefix(\""STR_PATH2"\");\n"
	"\n"
	"# System call constansts\n"
	"#   Used for system call support tests\n"
	"predicate is-socket-minor   = eq(1);\n"
	"predicate is-bind-minor     = eq(2);\n"
	"\n"
	"# File attribute constants\n"
	"#   Used for file attribute tests\n"
	"predicate __isreg           = mask(S_IFMT, S_IFREG);\n"
	"predicate __isdir           = mask(S_IFMT, S_IFDIR);\n"
	"predicate is-file           = __isreg(file-mode);\n"
	"predicate is-dir            = __isdir(file-mode);\n"
	"predicate is-file-testuid   = is-testuid(file-uid);\n"
	"predicate is-file-rootuid   = is-root(file-uid);\n"
	"predicate is-file-testgid   = is-testgid(file-gid);\n"
	"predicate is-file-rootgid   = is-root(file-gid);\n"
	"predicate is-inoA           = eq(%d);\n"	// fprintf with inode of audit.conf
	"predicate is-inoB           = eq(%d);\n"	// fprintf with inode + 1 of audit.conf
	"predicate is-file-inoA      = is-inoA(file-ino);\n"
	"predicate is-file-inoB      = is-inoB(file-ino);\n"
	"predicate is-devA           = eq(%lld);\n"	// fprintf with device of audit.conf
	"predicate is-devB           = eq(%lld);\n"	// fprintf with device + 1 of audit.conf
	"predicate is-file-devA      = is-devA(file-dev);\n"
	"predicate is-file-devB      = is-devB(file-dev);\n"
	"predicate is-devmajorA      = eq(%d);\n"	// fprintf with device major of audit.conf
	"predicate is-devmajorB      = eq(%d);\n"	// fprintf with device major + 1 of audit.conf
	"predicate is-devminorA      = eq(%d);\n"	// fprintf with device minor of audit.conf
	"predicate is-devminorB      = eq(%d);\n"	// fprintf with device minor + 1 of audit.conf
	"predicate is-dev-devmajorA  = is-devmajorA(dev-major);\n"
	"predicate is-dev-devmajorB  = is-devmajorB(dev-major);\n"
	"predicate is-dev-devminorA  = is-devminorA(dev-minor);\n"
	"predicate is-dev-devminorB  = is-devminorB(dev-minor);\n"
	"\n"
	"# Socket attribute constants\n"
	"#   Used for socket attribute tests\n"
	"predicate is-inet           = eq(AF_INET);\n"
	"predicate is-inet6          = eq(AF_INET6);\n"
	"predicate is-sock-inet      = is-inet(sock-family);\n"
	"predicate is-sock-inet6     = is-inet6(sock-family);\n"
	"predicate is-stream         = eq(SOCK_STREAM);\n"
	"predicate is-dgram          = eq(SOCK_DGRAM);\n"
	"predicate is-sock-stream    = is-stream(sock-type);\n"
	"predicate is-sock-dgram     = is-dgram(sock-type);\n"
	"\n"
	"# Netlink attribute constants\n"
	"#   Used for netlink attribute tests\n"
	"predicate is-nl-newroute    = eq(RTM_NEWROUTE);\n"
	"predicate is-nl-newaddr     = eq(RTM_NEWADDR);\n"
	"predicate is-nl-request     = mask(NLM_F_REQUEST, 1);\n"
	"predicate is-nl-norequest   = mask(NLM_F_REQUEST, 0);\n"
	"\n"
	"# Set collections\n"
	"#   Used for set tests\n"
	"set dir-set1 = {\n"
	"   \"/etc\", \"/home\", \"/root\"\n"
	"};\n"
	"set dir-set2 = {\n"
	"   \"/home\", \"/etc\", \"/root\"\n"
	"};\n"
	"set dir-set3 = {\n"
	"   \"/home\", \"/root\", \"/etc\"\n"
	"};\n"
	"set dir-set4 = {\n"
	"   \"/home\", \"/root\"\n"
	"};\n"
	"predicate is-dir-set1       = prefix(@dir-set1);\n"
	"predicate is-dir-set2       = prefix(@dir-set2);\n"
	"predicate is-dir-set3       = prefix(@dir-set3);\n"
	"predicate is-dir-set4       = prefix(@dir-set4);\n"
	"\n"
	"# Configurable test filters\n"
	"include \"filter_test.conf\";\n";


/**********************************************************************
**  Function: init_audit_conf
**    Sets the audit configuration for the tests
**
**   1) Backs up the audit configuration files that will be changed.
**   2) Creates the new audit configuration files.
**
**********************************************************************/
int init_audit_conf(uid_t uid, gid_t gid) {

	FILE *file = NULL;

	struct stat fs, ds;

	int i, j, k;
	char *devtype[4] = {"hd", "sd", "dasd", "iseries/vd"};
	char devname[32];
	int devfound = 0;


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

	// Get information about audit.conf for use in tests
	printf5("Getting information about file '%s'\n", AUDIT_CONF);
	if (stat(AUDIT_CONF, &fs) < 0) {
		printf1("ERROR: stat failed for file '%s' - errno=%d %s\n", AUDIT_CONF, errno, strerror(errno));
		return -1;
	}

	printf5("Searching for '%s' device name\n", AUDIT_CONF);
	for (i = 0; i < sizeof(devtype); i++) {
		for (j = 0; j < 26; j++) {
			for (k = 1; k < 32; k++) {
				sprintf(devname, "/dev/%s%c%d", devtype[i], j + 'a', k);
				if (stat(devname, &ds) == 0) {
					if ((major(ds.st_rdev) == major(fs.st_dev)) &&
					    (minor(ds.st_rdev) == minor(fs.st_dev))) {
						strcpy(test_devname, devname);
						devfound = 1;
						break;
					}
				}
			};

			if (devfound)
				break;
		}

		if (devfound)
			break;
	};
	if (!devfound) {
		printf1("ERROR: Unable to find '%s' device name\n", AUDIT_CONF);
		return -1;
	}
	printf5("Using '%s' as device name\n", test_devname);

	// Open and write the new audit.conf file
	printf5("Writing new '%s' configuration\n", AUDIT_CONF);
	file = fopen(AUDIT_CONF, "w");
	if (!file) {
		printf1("ERROR: fopen failed for '%s' - errno=%d %s\n", AUDIT_CONF, errno, strerror(errno));
		return -1;
	}
	fprintf(file, audit_conf);
	fclose(file);

	// Open and write the new filter.conf file
	printf5("Writing new '%s' configuration\n", FILTER_CONF);
	file = fopen(FILTER_CONF, "w");
	if (!file) {
		printf1("ERROR: fopen failed for '%s' - errno=%d %s\n", FILTER_CONF, errno, strerror(errno));
		return -1;
	}
	fprintf(file, filter_conf,
		login_uid, login_uid,
		uid, uid,
		gid, gid,
		fs.st_ino, fs.st_ino + 1,
		fs.st_dev, fs.st_dev + 1,
		major(fs.st_dev), major(fs.st_dev) + 1,
		minor(fs.st_dev), minor(fs.st_dev) + 1);
	fclose(file);

	// Create an empty filter_test.conf file
	printf5("Writing new '%s' configuration\n", FILTER_TEST_CONF);
	file = fopen(FILTER_TEST_CONF, "w");
	if (!file) {
		printf1("ERROR: fopen failed for '%s' - errno=%d %s\n", FILTER_TEST_CONF, errno, strerror(errno));
		return -1;
	}

	fprintf(file, "# Empty file\n");
	fclose(file);

	return 0;
}


/**********************************************************************
**  Function: term_audit_conf
**    Restores the original audit configuration
**
**   1) Deletes and restores the audit configuration files.
**
**********************************************************************/
int term_audit_conf(void) {

	if (!init)
		return 0;

	printf5("Deleting file '%s'\n", FILTER_TEST_CONF);
	unlink(FILTER_TEST_CONF);

	printf5("Calling restoreFile for file '%s'\n", AUDIT_CONF);
	restoreFile(AUDIT_CONF);
	printf5("Calling restoreFile for file '%s'\n", FILTER_CONF);
	restoreFile(FILTER_CONF);

	reloadAudit();

	init = 0;

	return 0;
}


/**********************************************************************
**  Function: set_filter_conf
**    Sets the filter statement to be tested
**
**   1) Creates a new filter_test.conf file using the supplied filter.
**
**********************************************************************/
int set_filter_conf(char *filter) {

	FILE *file = NULL;

	printf5("Writing new '%s' configuration\n", FILTER_TEST_CONF);
	file = fopen(FILTER_TEST_CONF, "w");
	if (!file) {
		printf1("ERROR: fopen failed for '%s' - errno=%d %s\n", FILTER_TEST_CONF, errno, strerror(errno));
		return -1;
	}

	fprintf(file, filter);
	fclose(file);

	return 0;
}
