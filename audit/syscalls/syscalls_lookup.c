/* =======================================================================
 *   (c) Copyright Hewlett-Packard Development Company, L.P., 2005
 *   Written by Amy Griffis <amy.griffis@hp.com>
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

#include "includes.h"
#include "syscalls.h"
#include <regex.h>

/*
 * Syscall Test Lookup Table
 */
struct syscall_tests {
    char        *testname;
    int         (*testp)(struct audit_data *, int, int);
};

struct syscall_variations {
    char        *varname;
    int		varnum;
};

static struct syscall_tests syscall_table[] = {
    { "setfsgid32", &test_setfsgid32 }, 
    { "setfsgid", &test_setfsgid }, 
    { "setfsuid32", &test_setfsuid32 }, 
    { "setfsuid", &test_setfsuid }, 
};

static struct syscall_variations syscall_vtable[] = {
    { "basic", TESTSC_BASIC },
    { "file", TESTSC_FILE },
    { "symlink", TESTSC_SYMLINK },
    { "modify", TESTSC_MODIFY },
    { "nomodify", TESTSC_NOMODIFY },
};

int (*lookup_testcase(char *testcase))(struct audit_data *, int, int)
{
    int i;

    for (i = 0; i < sizeof(syscall_table)/sizeof(syscall_table[0]); i++)
	if (!strcmp(testcase, syscall_table[i].testname)) {
	    return syscall_table[i].testp;
	}
    fprintf(stderr, "Error: testcase not found: %s\n", testcase);
    return NULL;
}

int lookup_variation(char *variation)
{
    int i;

    for (i = 0; i < sizeof(syscall_vtable)/sizeof(syscall_vtable[0]); i++)
	if (!strcmp(variation, syscall_vtable[i].varname)) {
	    return syscall_vtable[i].varnum;
	}
    fprintf(stderr, "Error: variation not found: %s\n", variation);
    return -1;
}

char *lookup_sysname(char *testname)
{
    char *sysname = NULL;
    char *regex = "^([^-]*)(-.*)?";
    regex_t pbuf;
    regmatch_t *pmatch;
    int nmatch, syslen;

    errno = 0;
    if (regcomp(&pbuf, regex, REG_EXTENDED) != 0) {
	fprintf(stderr, "Error: unable to compile regex %s: %s\n", regex,
		strerror(errno));
	return NULL;
    }
    
    nmatch = pbuf.re_nsub + 1;
    pmatch = malloc(sizeof(regmatch_t) * nmatch);
    if (!pmatch) {
	fprintf(stderr, "Error: malloc(): %s\n", strerror(errno));
	return NULL;
    }

    if (regexec(&pbuf, testname, nmatch, pmatch, 0) != 0) {
	fprintf(stderr, "Error: incorrect testname format: %s\n", testname);
	goto exit;
    }
    syslen = pmatch[nmatch-2].rm_eo - pmatch[nmatch-2].rm_so;
    sysname = strndup(testname + pmatch[nmatch-2].rm_so, syslen + 1);
    sysname[syslen] = '\0';

exit:
    free(pmatch);
    return sysname;
}
