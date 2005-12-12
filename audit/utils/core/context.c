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
#include <libaudit.h>
#include <time.h>

void context_init(struct audit_data *context, int evtype)
{
    memset(context, 0, sizeof(struct audit_data));
    context->type     = evtype;
    context->pid      = getpid();
    context->loginuid = audit_getloginuid();

    fprintf(stderr, "Setting context pid: %i\n", context->pid);
    fprintf(stderr, "Setting context loginuid: %u\n", context->loginuid);
}

int context_initsyscall(struct audit_data *context, char *sname)
{
    int rc = 0;
    char *sysname;
    int sysnum;

    errno = 0;
    sysname = strdup(sname);
    if (!sysname) {
	rc = -1;
	fprintf(stderr, "Error: strdup(): %s\n", strerror(errno));
    }

    sysnum = audit_name_to_syscall(sysname, get_auditarch());
    if (sysnum < 0) {
	rc = -1;
	fprintf(stderr, "Error: unable to translate \"%s\" to syscall number\n",
		sysname);
	goto exit;
    }

    context->u.syscall.sysname = sysname;
    context->u.syscall.sysnum  = sysnum;
    context->u.syscall.arch    = TS_BUILD_ARCH;

    fprintf(stderr, "Setting context sysnum: %i\n", context->u.syscall.sysnum);

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

    fprintf(stderr, 
	    "Setting context uids: uid: %u euid: %u suid: %u fsuid: %u\n", 
	    context->uid, context->euid, context->suid, context->fsuid);
    fprintf(stderr, 
	    "Setting context gids: gid: %u egid: %u sgid: %u fsgid: %u\n", 
	    context->gid, context->egid, context->sgid, context->fsgid);

exit:
    if (rc < 0)
	fprintf(stderr, "Error: unable set context process identifiers.\n");
    return rc;
}

/* allow fsgid and fsuid to be explicitly set */
void context_setfsgid(struct audit_data *context, int fsgid)
{
    context->fsgid = fsgid;
    fprintf(stderr, "Setting context fsgid (explicitly): %i\n", context->fsgid);
}

void context_setfsuid(struct audit_data *context, int fsuid)
{
    context->fsuid = fsuid;
    fprintf(stderr, "Setting context fsuid (explicitly): %i\n", context->fsuid);
}

void context_setpid(struct audit_data *context, int pid)
{
    context->pid = pid;
    fprintf(stderr, "Setting context pid: %i\n", context->pid);
}

void context_setexperror(struct audit_data *context, int error)
{
    context->experror = -error;
    fprintf(stderr, "Setting operation expected error: %i\n", 
	    context->experror);
}

void context_setbegin(struct audit_data *context)
{
    context->begin_time = time(NULL);
    fprintf(stderr, "Setting operation start time: %s", 
	    ctime(&(context->begin_time)));
}

void context_setend(struct audit_data *context)
{
    context->end_time = time(NULL);
    fprintf(stderr, "Setting operation end time: %s", 
	    ctime(&(context->end_time)));
}

/*
 * Syscall Related Context
 */

void context_setresult(struct audit_data * context, long exit, int error)
{
    if ((exit < 0) && (exit > -256)) {
	context->success = 0;
	context->error = context->u.syscall.exit = -error;

	fprintf(stderr, "Setting context error: %i\n", context->error);
    } else {
	context->success = 1;
	context->u.syscall.exit = exit;
    }

    fprintf(stderr, "Setting context exit: %i\n", context->u.syscall.exit);
    fprintf(stderr, "Setting context success: %u\n", context->success);
}

void context_setipc(struct audit_data *context, int qbytes, 
		    int uid, int gid, long msgflg)
{
    context->type |= AUDIT_MSG_IPC;
    context->u.syscall.ipc_qbytes = qbytes;
    context->u.syscall.ipc_uid = uid;
    context->u.syscall.ipc_gid = gid;
    context->u.syscall.ipc_mode = msgflg;

    fprintf(stderr, "Setting context ipc_qbytes: %x\n", 
	    context->u.syscall.ipc_qbytes);
    fprintf(stderr, "Setting context ipc_uid: %u\n", 
	    context->u.syscall.ipc_uid);
    fprintf(stderr, "Setting context ipc_gid: %u\n", 
	    context->u.syscall.ipc_gid);
    fprintf(stderr, "Setting context ipc_mode: %x\n", 
	    context->u.syscall.ipc_mode);
}

void context_setsockaddr(struct audit_data *context, unsigned char *buf, 
			 size_t len)
{
    int i;
    unsigned char* ptr = context->u.syscall.sockaddr;

    context->type |= AUDIT_MSG_SOCKADDR;
    for (i = 0; i < len; i++) {
	sprintf(ptr, "%.2X", buf[i]);
	ptr+=2;
    }
    *ptr = '\0';
    fprintf(stderr, "Setting context sockaddr: %s\n",
	    context->u.syscall.sockaddr);
}

int context_setcwd(struct audit_data *context)
{
    if (getcwd(context->u.syscall.fs_cwd, PATH_MAX) == NULL) {
	fprintf(stderr, "Error: unable set context cwd.\n");
	return -1;
    }
    context->type |= AUDIT_MSG_CWD;
    fprintf(stderr, "Setting context cwd: %s\n", context->u.syscall.fs_cwd);
    return 0;
}

void context_setsobj(struct audit_data *context, char *obj)
{
    context->type |= AUDIT_MSG_PATH;
    strncpy(context->u.syscall.fs_sobj, obj, PATH_MAX);
    fprintf(stderr, "Setting context object: %s\n", 
	    context->u.syscall.fs_sobj);
}

void context_settobj(struct audit_data *context, char *obj)
{
    context->type |= AUDIT_MSG_PATH;
    strncpy(context->u.syscall.fs_tobj, obj, PATH_MAX);
    fprintf(stderr, "Setting context object: %s\n", 
	    context->u.syscall.fs_tobj);
}

void context_setwatch(struct audit_data *context, char *obj)
{
    context->type |= AUDIT_MSG_WATCH;
    strncpy(context->u.syscall.fs_watch, obj, PATH_MAX);
    fprintf(stderr, "Setting context watch: %s\n", context->u.syscall.fs_watch);
}

void context_setarg(struct audit_data *context, int argnum, int arg)
{
    switch(argnum) {
    case 0:
	context->type |= AUDIT_MSG_ARG0;
	break;
    case 1:
	context->type |= AUDIT_MSG_ARG1;
	break;
    case 2:
	context->type |= AUDIT_MSG_ARG2;
	break;
    case 3:
	context->type |= AUDIT_MSG_ARG3;
	break;
    default:
	return;
    }
    context->u.syscall.arg[argnum] = arg;
    fprintf(stderr, "Setting context arg[%i]: %x\n", argnum,
	    context->u.syscall.arg[argnum]);
}

char *context_getcwd(struct audit_data *context)
{
    return context->u.syscall.fs_cwd;
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
	if (context->type & AUDIT_MSG_SOCKADDR)
	    fprintf(stderr, "\tsockaddr  : %s\n", syscall->sockaddr); 
	if (context->type & AUDIT_MSG_CWD)
	    fprintf(stderr, "\tfs_cwd    : %s\n", syscall->fs_cwd ?: "(null)");
	if (context->type & AUDIT_MSG_WATCH)
	    fprintf(stderr, "\tfs_watch  : %s\n", syscall->fs_watch ?:"(null)");
	if (context->type & AUDIT_MSG_PATH) {
	    fprintf(stderr, "\tfs_sobj   : %s\n", syscall->fs_sobj ?: "(null)");
	    fprintf(stderr, "\tfs_tobj   : %s\n", syscall->fs_tobj ?: "(null)");
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
