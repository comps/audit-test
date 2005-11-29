/* =======================================================================
 * (c) Copyright Hewlett-Packard Development Company, L.P., 2005
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
#include <libaudit.h>
#include <time.h>

void context_init(struct audit_data *context, int evtype)
{
    memset(context, 0, sizeof(struct audit_data));
    context->type     = evtype;
    context->pid      = getpid();
    context->loginuid = audit_getloginuid();
}

int context_initsyscall(struct audit_data *context, char *testname)
{
    int rc = 0;
    char *sysname;
    int sysnum;
    char cwd[PATH_MAX];

    errno = 0;
    sysname = strdup(testname);
    if (!sysname) {
	rc = -1;
	fprintf(stderr, "Error: strdup(): %s\n", strerror(errno));
    }

    sysnum = audit_name_to_syscall(sysname, audit_detect_machine());
    if (sysnum < 0) {
	rc = -1;
	fprintf(stderr, "Error: unable to translate \"%s\" to syscall number\n",
		sysname);
	goto exit;
    }

    errno = 0;
    if (getcwd(cwd, PATH_MAX) == NULL) {
	rc = -1;
	fprintf(stderr, "Error: unable to get current working directory: %s\n",
		strerror(errno));
	goto exit;
    }

    context->u.syscall.sysname = sysname;
    context->u.syscall.sysnum  = sysnum;
    context->u.syscall.arch    = TS_BUILD_ARCH;
    context->u.syscall.exit    = 0;
    strncpy(context->u.syscall.cwd, cwd, PATH_MAX);

exit:
    if (rc < 0)
	fprintf(stderr, "Error: unable initialize context syscall values.\n");
    return rc;
}

int context_setidentifiers(struct audit_data *context)
{
    int rc = 0;

    rc = getresuid(&context->uid, &context->euid, &context->suid);
    if (rc < 0)
        goto exit;
    /* fsuid follows euid, unless set explicitly */
    context->fsuid = context->euid;

    rc = getresgid(&context->gid, &context->egid, &context->sgid);
    /* fsgid follows egid, unless set explicitly */
    context->fsgid = context->egid;

exit:
    if (rc < 0)
	fprintf(stderr, "Error: unable set context process identifiers.\n");
    return rc;
}

void context_setbegin(struct audit_data *context)
{
    context->begin_time = time(NULL);
    fprintf(stderr, "Operation began at %s", ctime(&(context->begin_time)));
}

void context_setend(struct audit_data *context)
{
    context->end_time = time(NULL);
    fprintf(stderr, "Operation ended at %s", ctime(&(context->end_time)));
}

/*
 * Syscall Related Context
 */

void context_setresult(struct audit_data * context, int exit, int error)
{
    if (exit < 0) {
	context->success = 0;
	context->error = context->u.syscall.exit = -errno;
    } else {
	context->success = 1;
	context->u.syscall.exit = exit;
    }
}

/* context_setipc() should be called after context_setidentifiers() if
 * not explicitly setting uid and gid */
void context_setipc(struct audit_data *context, int qbytes, 
		    int uid, int gid, long msgflg)
{
    context->type |= AUDIT_MSG_IPC;
    context->u.syscall.ipc_qbytes = qbytes;
    context->u.syscall.ipc_uid = uid;
    context->u.syscall.ipc_gid = gid;
    context->u.syscall.ipc_mode = msgflg;
}

int context_setcwd(struct audit_data *context)
{
    if (getcwd(context->u.syscall.cwd, PATH_MAX) == NULL) {
	fprintf(stderr, "Error: unable set context cwd.\n");
	return -1;
    }
    context->type |= AUDIT_MSG_CWD;
    return 0;
}

void context_setfilterkey(struct audit_data *context, char *key)
{
    strncpy(context->u.syscall.fk, key, PATH_MAX);
}

char *context_getcwd(struct audit_data *context)
{
    return context->u.syscall.cwd;
}

void context_dump(const struct audit_data *context)
{
    fprintf(stderr, "\ttype      : %u\n", context->type);
    fprintf(stderr, "\tserial    : %u\n", context->serial);
    fprintf(stderr, "\ttimestamp : %s", ctime(&(context->timestamp)));
    fprintf(stderr, "\tbegin     : %s", ctime(&(context->begin_time)));
    fprintf(stderr, "\tend       : %s", ctime(&(context->end_time)));
    fprintf(stderr, "\tpid       : %i\n", context->pid);
    fprintf(stderr, "\tloginuid	 : %u\n", context->loginuid);
    fprintf(stderr, "\tuid       : %u\n", context->uid);
    fprintf(stderr, "\tgid       : %u\n", context->gid);
    fprintf(stderr, "\teuid      : %u\n", context->euid);
    fprintf(stderr, "\tsuid      : %u\n", context->suid);
    fprintf(stderr, "\tfsuid     : %u\n", context->fsuid);
    fprintf(stderr, "\tegid      : %u\n", context->egid);
    fprintf(stderr, "\tsgid      : %u\n", context->sgid);
    fprintf(stderr, "\tfsgid     : %u\n", context->fsgid);
    fprintf(stderr, "\tsuccess	 : %u\n", context->success);
    fprintf(stderr, "\terror     : %i\n", context->error);
    fprintf(stderr, "\texperror  : %i\n", context->experror);

    if (context->type & AUDIT_MSG_SYSCALL) {
        const struct audit_syscall *syscall = &context->u.syscall;

	fprintf(stderr, "\tsyscall   : %s\n", syscall->sysname ?: "(null)");
	fprintf(stderr, "\tsysnum    : %i\n", syscall->sysnum);
	fprintf(stderr, "\tarch      : %x\n", syscall->arch);
	fprintf(stderr, "\texit      : %i\n", syscall->exit);

	if (context->type & AUDIT_MSG_IPC) {
	    fprintf(stderr, "\tipc_qbytes: %x\n", syscall->ipc_qbytes); 
	    fprintf(stderr, "\tipc_uid   : %u\n", syscall->ipc_uid);
	    fprintf(stderr, "\tipc_gid   : %u\n", syscall->ipc_gid);
	    fprintf(stderr, "\tipc_mode  : %x\n", syscall->ipc_mode);
	}
	if (context->type & AUDIT_MSG_CWD) {
	    fprintf(stderr, "\tcwd       : %s\n", syscall->cwd ?: "(null)");
	    fprintf(stderr, "\tfk        : %s\n", syscall->fk ?: "(null)");
	}
	if (context->type & AUDIT_MSG_SOCKADDR) {
	    /* fprintf(stderr, "\tsockaddr  : %s\n", syscall->sockaddr); */
	}
    } else if (context->type == AUDIT_MSG_USER) {
	fprintf(stderr, "\ttext: %s\n", context->u.user.buf);
    }
}

void context_release(struct audit_data *context)
{
    if (context->type == AUDIT_MSG_SYSCALL && context->u.syscall.sysname)
	free(context->u.syscall.sysname);
}
