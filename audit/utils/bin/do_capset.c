/*  (c) Copyright Hewlett-Packard Development Company, L.P., 2007
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of version 2 the GNU General Public License as
 *  published by the Free Software Foundation.
 *  
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *  
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "includes.h"
#include <sys/capability.h>

/* limit number of supplementary capabilities to 50 */
#define MAX_CAPS 50

int translate_capability(char *);

int main(int argc, char **argv)
{
    int exitval, result, i, nr_caps = 0;
    cap_t caps;
    cap_value_t cap_list[MAX_CAPS] = { 0 };

    if ((argc < 2) || (argc - 1 > MAX_CAPS)){
	fprintf(stderr, "Usage:\n%s <capability> [capability]... (max %d)\n",
		argv[0], MAX_CAPS);
	return TEST_ERROR;
    }

    for (i = 1; i < argc; i++) {
	exitval = translate_capability(argv[i]);

	if (exitval < 0) {
	    fprintf(stderr, "Usage:\n%s: unknown capability: %s\n", argv[0], argv[i]);
	    return TEST_ERROR;
	}

	cap_list[nr_caps] = exitval;
	nr_caps++;
    }

    caps = cap_get_proc();
    if (!caps) {
	perror("do_capset: cap_get_proc");
	return TEST_ERROR;
    }
    cap_set_flag(caps, CAP_EFFECTIVE, nr_caps, cap_list , CAP_SET);

    errno = 0;
    exitval = cap_set_proc(caps);
    result = exitval < 0;

    cap_free(caps);

    printf("%d %d %d\n", result, result ? errno : exitval, getpid());
    return result;
}

int translate_capability(char *capstr) {
    if (!strcmp(capstr, "cap_chown")) {
	return CAP_CHOWN;
    } else if (!strcmp(capstr, "cap_dac_override")) {
	return CAP_DAC_OVERRIDE;
    } else if (!strcmp(capstr, "cap_dac_read_search")) {
	return CAP_DAC_READ_SEARCH;
    } else if (!strcmp(capstr, "cap_fowner")) {
	return CAP_FOWNER;
    } else if (!strcmp(capstr, "cap_fsetid")) {
	return CAP_FSETID;
    } else if (!strcmp(capstr, "cap_kill")) {
	return CAP_KILL;
    } else if (!strcmp(capstr, "cap_setgid")) {
	return CAP_SETGID;
    } else if (!strcmp(capstr, "cap_setuid")) {
	return CAP_SETUID;
    } else if (!strcmp(capstr, "cap_setcap")) {
	return CAP_SETPCAP;
    } else if (!strcmp(capstr, "cap_linux_immutable")) {
	return CAP_LINUX_IMMUTABLE;
    } else if (!strcmp(capstr, "cap_net_bind_service")) {
	return CAP_NET_BIND_SERVICE;
    } else if (!strcmp(capstr, "cap_net_broadcast")) {
	return CAP_NET_BROADCAST;
    } else if (!strcmp(capstr, "cap_net_admin")) {
	return CAP_NET_ADMIN;
    } else if (!strcmp(capstr, "cap_net_raw")) {
	return CAP_NET_RAW;
    } else if (!strcmp(capstr, "cap_ipc_lock")) {
	return CAP_IPC_LOCK;
    } else if (!strcmp(capstr, "cap_ipc_owner")) {
	return CAP_IPC_OWNER;
    } else if (!strcmp(capstr, "cap_sys_module")) {
	return CAP_SYS_MODULE;
    } else if (!strcmp(capstr, "cap_sys_rawio")) {
	return CAP_SYS_RAWIO;
    } else if (!strcmp(capstr, "cap_sys_chroot")) {
	return CAP_SYS_CHROOT;
    } else if (!strcmp(capstr, "cap_sys_ptrace")) {
	return CAP_SYS_PTRACE;
    } else if (!strcmp(capstr, "cap_sys_pacct")) {
	return CAP_SYS_PACCT;
    } else if (!strcmp(capstr, "cap_sys_admin")) {
	return CAP_SYS_ADMIN;
    } else if (!strcmp(capstr, "cap_sys_boot")) {
	return CAP_SYS_BOOT;
    } else if (!strcmp(capstr, "cap_sys_nice")) {
	return CAP_SYS_NICE;
    } else if (!strcmp(capstr, "cap_sys_resource")) {
	return CAP_SYS_RESOURCE;
    } else if (!strcmp(capstr, "cap_sys_time")) {
	return CAP_SYS_TIME;
    } else if (!strcmp(capstr, "cap_sys_tty_config")) {
	return CAP_SYS_TTY_CONFIG;
    } else if (!strcmp(capstr, "cap_mknod")) {
	return CAP_MKNOD;
    } else if (!strcmp(capstr, "cap_lease")) {
	return CAP_LEASE;
    } else if (!strcmp(capstr, "cap_audit_write")) {
	return CAP_AUDIT_WRITE;
    } else if (!strcmp(capstr, "cap_audit_control")) {
	return CAP_AUDIT_CONTROL;
    } else {
	return -1;
    }
}
