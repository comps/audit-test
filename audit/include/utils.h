/**********************************************************************
**   Copyright (C) International Business Machines  Corp., 2003
**   (c) Copyright Hewlett-Packard Development Company, L.P., 2005
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
**********************************************************************/

#ifndef _UTILS_H
#define _UTILS_H

/*
 * Audit Interaction
 */
char *autest_add_watch(const char *);
void autest_rem_watch(const char *, const char *);

/*
 * System Info Collection
 */
int get_auditarch(void);
uid_t gettestuid(void);
gid_t gettestgid(void);

int seteuid_test(void);
int setresuid_test(void);
int setuidresgid_test(void);
int setuidresgid_root(void);

/*
 * File Content Preservation
 */
int backupFile(char*);
int restoreFile(char*);

/*
 * Temp Data Creation
 */
char *init_tempdir(mode_t, uid_t, gid_t);
char *init_tempfile(mode_t, uid_t, gid_t, char *);
char *init_tempswap(mode_t, uid_t, gid_t, size_t);
char *init_tempsym(char *, uid_t, gid_t);
void destroy_tempdir(char *);
void destroy_tempfile(char *);
void destroy_tempsym(char *);

int createTempUserName(char** user, int* uid, char** homedir);
int createTempGroupName( char** user, int* uid );

/*
 * System context accessor routines
 */
void context_init(struct audit_data *, int);
int context_initsyscall(struct audit_data *, char *);
int context_setidentifiers(struct audit_data *);
void context_settype(struct audit_data *, unsigned int);
void context_setfsgid(struct audit_data *, int);
void context_setfsuid(struct audit_data *, int);
void context_setpid(struct audit_data *, int);
void context_setexperror(struct audit_data *, int);
void context_setbegin(struct audit_data *);
void context_setend(struct audit_data *);
void context_setresult(struct audit_data *, long, int);
void context_setipc(struct audit_data *, int, int, int, long);
void context_setsockaddr(struct audit_data *, unsigned char *, size_t);
int context_setcwd(struct audit_data *);
void context_setsdir(struct audit_data *, char *);
void context_setsobj(struct audit_data *, char *);
void context_settdir(struct audit_data *, char *);
void context_settobj(struct audit_data *, char *);
void context_setwatch(struct audit_data *, char *);
void context_setdev(struct audit_data *, dev_t);
void context_setino(struct audit_data *, ino_t);
void context_setarg(struct audit_data *, int, int);
char *context_getcwd(struct audit_data *);
void context_dump(const struct audit_data *);
void context_release(struct audit_data *);

/*
 * Test Verification
 */
ts_exit verify_opresult(struct audit_data *, int);
ts_exit verify_logresult(struct audit_data *);

/*
 * Shell Command Utilities
 */
int run(char* command); 
char* mysprintf(char* fmt, ...);
#define RUNCOMMAND(cmd, ...) \
do {									\
    char* command = mysprintf(cmd, ## __VA_ARGS__);			\
    if ((rc = system(command)) == -1) {					\
	fprintf(stderr, "Error running command: [%s]\n", command);	\
    }									\
    free(command);							\
} while (0)								\

#define RUNCOMMANDORDIE(cmd, ...) \
do {									\
    char* command = mysprintf(cmd, ## __VA_ARGS__ );			\
    if ((rc = system(command)) == -1) {					\
	fprintf(stderr, "Error running command: [%s]\n", command);	\
	free(command);							\
	goto EXIT_CLEANUP;						\
    }									\
    free(command);							\
} while (0)

#endif	/* _UTILS_H */
