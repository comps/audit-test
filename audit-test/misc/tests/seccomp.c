/* this is a seccomp tester, which takes input from and provides data
 * for test_seccomp.bash , which then evaluates the results
 *
 * this tester has no notion of "pass" or "fail" (!!!), it simply runs stuff
 * and reports back the results
 *
 * this tester can exit with these exit codes:
 *   code 0 = tester finished successfully (wasn't killed or signaled)
 *   code 1 = argument/environment/setup/other unexpected error
 *   code 2 = SIGSYS triggered (and a custom handler was able to execute)
 *   code * = anything else (segfault, killed by kernel, ...)
 *
 * if this tester is still alive after calling the syscall, it prints out
 * info regarding execution on stdout, with values in the following order:
 *   <retval>   - (integer) return value of the syscall
 *   <errno>    - the syscall errno variable value as defined in errno.h
 *   <success>  - whether the syscall actually performed requested action,
 *                regardless of its return value
 *   <scerrno>  - testing errno/trace value used for the errno/trace seccomp
 *                action when setting up the rule
 *
 * ie. "0 0 1 1000", "-1000 0 0 1000", "-1 2 0 1000", ...
 *
 * for the actual syscall testing, dup2() was chosen because it has numeric
 * arguments (easy to check with libseccomp), does something verifiable
 * and does so with little process/system impact
 * open() as the second syscall represents an often-used syscall
 *
 * this file is split into several sections, with the syscall-specific logic
 * defined in isolated setup/exec/cleanup/success functions -
 * command line arguments then select the syscall, tested syscall action,
 * ruleset type (rule arrangement / options) and precedence type
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <signal.h>
#include <fcntl.h>
#include <errno.h>

#include <seccomp.h>

#include <unistd.h>


/*
 * definitions, global variables
 */

#define TESTING_SCERRNO 0x123

#define ARG_SYSCALL(x)    (strcmp(argv[1],x)==0)
#define ARG_ACTION(x)     (strcmp(argv[2],x)==0)
#define ARG_RULESET(x)    (strcmp(argv[3],x)==0)
#define ARG_ARGUMENT(x)   (strcmp(argv[4],x)==0)
#define ARG_PRECEDENCE(x) (strncmp(argv[5],x,strlen(x))==0)

/* an indicator whether the tested syscall triggered SIGSYS */
sig_atomic_t trapped = 0;


/*
 * data types
 */

/* struct used for testing syscalls,
 * setup is run before syscall, exec is supposed to run the syscall,
 * cleanup is run after syscall and success should return true if the
 * syscall succeeded, false otherwise
 * - setup/test/cleanup return -1 on error, non-negative value otherwise */
struct syscall_info {
    int syscall_nr;  /* syscall number */
    int ret_val;     /* syscall return value */
    int ret_errno;   /* syscall-set errno */
    int arg_idx;     /* index (from 0) of tested argument */
    int arg_val;     /* value of that argument (passed to syscall itself) */
    int (*setup)(struct syscall_info *);
    int (*exec)(struct syscall_info *);
    int (*cleanup)(struct syscall_info *);
    bool (*success)(struct syscall_info *);
    void *shared;    /* generic shared storage, usable by functions */
    scmp_filter_ctx *ctx;  /* for possible use in setup, exception addition */
};


/*
 * generic helper functions
 */

/* add additional seccomp rule exceptions (syscalls),
 * ie. syscalls needed for this C tester to exit properly
 * or per-syscall specific, but test-unrelated calls (from setup/cleanup) */
int add_scmp_exceptions(scmp_filter_ctx *ctx, int *scalls, int size)
{
    int i, rc;
    uint32_t def_action;

    if ((rc = seccomp_attr_get(*ctx, SCMP_FLTATR_ACT_DEFAULT, &def_action)) < 0)
        return rc;

    /* note: libseccomp tries to be smart, rule_add fails if rule action
     * equals to default action, so avoid this situation here - silently
     * do nothing, the exception is not needed, since it's allowed by default,
     * see RHBZ#1035748 */
    if (def_action == SCMP_ACT_ALLOW)
        return 0;

    for (i = 0; i < size; i++)
        if ((rc = seccomp_rule_add(*ctx, SCMP_ACT_ALLOW, scalls[i], 0)) < 0)
            return rc;

    return 0;
}

/* add exceptions specific to this tester */
int allow_tester_exit(scmp_filter_ctx *ctx)
{
    int scalls[] = { SCMP_SYS(fstat),         /* printf */
                     SCMP_SYS(fstat64),       /* printf, 32bit */
                     SCMP_SYS(mmap),          /* printf */
                     SCMP_SYS(mmap2),         /* printf, 32bit */
                     SCMP_SYS(write),         /* printf */
                     SCMP_SYS(sigreturn),     /* return from sigsys handler */
                     SCMP_SYS(rt_sigreturn),  /* return from sigsys handler */
                     SCMP_SYS(prctl),         /* for precedence testing */
                     SCMP_SYS(exit_group) };

    return add_scmp_exceptions(ctx, scalls, sizeof(scalls)/sizeof(int));
}

void sigsys_handler(int signum)
{
    trapped = 1;
}


/*
 * syscall testing functions
 */

/* each syscall needs to have (at least) setup, exec and success functions
 * and needs to set argument index/value in the setup */

/*** dup2 ***/
int dup2_setup(struct syscall_info *si)
{
    si->arg_idx = 0;
    si->arg_val = 0;

    /* allow fcntl in check */
    int scalls[] = { SCMP_SYS(fcntl), SCMP_SYS(fcntl64) };
    if (add_scmp_exceptions(si->ctx, scalls, 2) < 0)
        return -1;

    close(42);
    return 0;
}
int dup2_exec(struct syscall_info *si)
{
    si->ret_val = dup2(0, 42);
    si->ret_errno = errno;
    return 0;
}
bool dup2_success(struct syscall_info *si)
{
    return !(fcntl(42, F_GETFL) == -1 && errno == EBADF);
}

/*** open ***/
int open_setup(struct syscall_info *si)
{
    si->arg_idx = 1;
    si->arg_val = O_RDWR | O_CREAT;

    /* allow unlink in cleanup / access in check */
    int scalls[] = { SCMP_SYS(unlink), SCMP_SYS(access) };
    if (add_scmp_exceptions(si->ctx, scalls, 2) < 0)
        return -1;

    /* check for / remove possible existing test file */
    if (access("/tmp/seccomp_test_file", F_OK) == 0)
        if (unlink("/tmp/seccomp_test_file") == -1)
            return -1;
    return 0;
}
int open_exec(struct syscall_info *si)
{
    si->ret_val = open("/tmp/seccomp_test_file", O_RDWR | O_CREAT, 0644);
    si->ret_errno = errno;
    return 0;
}
int open_cleanup(struct syscall_info *si)
{
    if (access("/tmp/seccomp_test_file", F_OK) == 0)
        if (unlink("/tmp/seccomp_test_file") == -1)
            return -1;
    return 0;
}
bool open_success(struct syscall_info *si)
{
    return !access("/tmp/seccomp_test_file", F_OK);
}


/*
 * main
 */
int main(int argc, char **argv)
{
    scmp_filter_ctx ctx = NULL, ctx2 = NULL;
    uint32_t def_action;
    uint32_t syscall_action;

    struct syscall_info sinfo = { 0 };

    if (argc < 6) {
        fprintf(stderr,
                "usage: %s <syscall> <action> <ruleset> <arg> <precedence>\n"
                "\n"
                "syscall: dup2, open\n"
                "\n"
                "action:  kill, trap, errno, trace, allow\n"
                "\n"
                "ruleset:\n"
                "  none                 - no syscall-related rules\n"
                "  match                - simple rule matching the syscall\n"
                "  mismatch             - simple rule unrelated to the syscall\n"
                "\n"
                "arg:\n"
                "  none                 - no argument matching in the rule\n"
                "  match                - rule with arg matching the executed syscall\n"
                "  mismatch             - rule with arg unrelated to the syscall\n"
                "\n"
                "precedence:\n"
                "  none                 - no precedence testing performed\n"
                "  x-after-allow        - testing rule added after an ALLOW rule\n"
                "  x-before-allow       - testing rule added before an ALLOW rule\n"
                "  x-after-trap         - testing rule added after a TRAP rule\n"
                "  x-before-trap        - testing rule added before a TRAP rule\n"
                "  x-after-kill         - testing rule added after a KILL rule\n"
                "  x-before-kill        - testing rule added before a KILL rule\n"
                , argv[0]);
        exit(1);
    }

    /* select syscall */
    if (ARG_SYSCALL("dup2")) {
        sinfo.syscall_nr = SCMP_SYS(dup2);
        sinfo.setup = &dup2_setup;
        sinfo.exec = &dup2_exec;
        sinfo.success = &dup2_success;
    } else if (ARG_SYSCALL("open")) {
        sinfo.syscall_nr = SCMP_SYS(open);
        sinfo.setup = &open_setup;
        sinfo.exec = &open_exec;
        sinfo.cleanup = &open_cleanup;
        sinfo.success = &open_success;
    }

    /* select action */
    if (ARG_ACTION("kill")) {
        syscall_action = SCMP_ACT_KILL;
        def_action = SCMP_ACT_ALLOW;
    } else if (ARG_ACTION("trap")) {
        syscall_action = SCMP_ACT_TRAP;
        def_action = SCMP_ACT_ALLOW;
    } else if (ARG_ACTION("errno")) {
        /* use a reasonably unique return value */
        syscall_action = SCMP_ACT_ERRNO(TESTING_SCERRNO);
        def_action = SCMP_ACT_ALLOW;
    } else if (ARG_ACTION("trace")) {
        syscall_action = SCMP_ACT_TRACE(TESTING_SCERRNO);
        def_action = SCMP_ACT_ALLOW;
    } else if (ARG_ACTION("allow")) {
        /* we need to use non-allow default action here
         * and allow tester-related syscalls later */
        syscall_action = SCMP_ACT_ALLOW;
        def_action = SCMP_ACT_TRAP;
    } else {
        exit(1);
    }

    /* set up SIGSYS handler */
    if (signal(SIGSYS, &sigsys_handler) == SIG_ERR)
        exit(1);

    /* init the primary seccomp filter structure */
    ctx = seccomp_init(def_action);
    if (ctx == NULL)
        exit(1);

    /* allow this tester to exit the way we want */
    if (allow_tester_exit(&ctx) < 0)
        exit(1);

    /* call syscall setup
     * - this has to be done before ruleset additions, since setup
     *   also sets argument index and value */
    sinfo.ctx = &ctx;
    if (sinfo.setup(&sinfo) < 0)
        exit(1);

    /* add rules according to ruleset type */
    if (ARG_RULESET("none")) {
    } else if (ARG_RULESET("match")) {
        if (ARG_ARGUMENT("none")) {
            if (seccomp_rule_add(ctx, syscall_action, sinfo.syscall_nr, 0) < 0) {
                exit(1);
            }
        } else if (ARG_ARGUMENT("match")) {
            if (seccomp_rule_add(ctx, syscall_action, sinfo.syscall_nr, 1,
                                 SCMP_CMP(sinfo.arg_idx, SCMP_CMP_EQ,
                                          sinfo.arg_val)) < 0) {
                exit(1);
            }
        } else if (ARG_ARGUMENT("mismatch")) {
            if (seccomp_rule_add(ctx, syscall_action, sinfo.syscall_nr, 1,
                                 SCMP_CMP(sinfo.arg_idx, SCMP_CMP_EQ,
                                          !sinfo.arg_val)) < 0) {
                exit(1);
            }
        }
    /* TODO: might not work as !syscall_nr is 0 and adding 0 might not make sense */
    } else if (ARG_RULESET("mismatch")) {
        if (ARG_ARGUMENT("none")) {
            if (seccomp_rule_add(ctx, syscall_action, !sinfo.syscall_nr, 0) < 0) {
                exit(1);
            }
        } else if (ARG_ARGUMENT("match")) {
            if (seccomp_rule_add(ctx, syscall_action, !sinfo.syscall_nr, 1,
                                 SCMP_CMP(sinfo.arg_idx, SCMP_CMP_EQ,
                                          sinfo.arg_val)) < 0) {
                exit(1);
            }
        } else if (ARG_ARGUMENT("mismatch")) {
            if (seccomp_rule_add(ctx, syscall_action, !sinfo.syscall_nr, 1,
                                 SCMP_CMP(sinfo.arg_idx, SCMP_CMP_EQ,
                                          !sinfo.arg_val)) < 0) {
                exit(1);
            }
        }
    } else {
        exit(1);
    }

    /* add precedence rules to a separate filter structure */
    if (ARG_PRECEDENCE("none")) {
    } else if (ARG_PRECEDENCE("x-after-allow") || ARG_PRECEDENCE("x-before-allow")) {
        ctx2 = seccomp_init(SCMP_ACT_TRAP);
        if (ctx2 == NULL ||
            seccomp_rule_add(ctx2, SCMP_ACT_ALLOW, sinfo.syscall_nr, 0) < 0) {
            exit(1);
        }
    } else if (ARG_PRECEDENCE("x-after-trap") || ARG_PRECEDENCE("x-before-trap")) {
        ctx2 = seccomp_init(SCMP_ACT_ALLOW);
        if (ctx2 == NULL ||
            seccomp_rule_add(ctx2, SCMP_ACT_TRAP, sinfo.syscall_nr, 0) < 0) {
            exit(1);
        }
    } else if (ARG_PRECEDENCE("x-after-kill") || ARG_PRECEDENCE("x-before-kill")) {
        ctx2 = seccomp_init(SCMP_ACT_ALLOW);
        if (ctx2 == NULL ||
            seccomp_rule_add(ctx2, SCMP_ACT_KILL, sinfo.syscall_nr, 0) < 0) {
            exit(1);
        }
    } else {
        exit(1);
    }

    /* load seccomp rules */
    /* load precedence here, not in ARG_PRECEDENCE above, since rule addition
     * uses malloc/realloc, etc. */
    if (ctx2 != NULL && ARG_PRECEDENCE("x-after-"))
        if (seccomp_load(ctx2) < 0)
            exit(1);
    if (seccomp_load(ctx) < 0)
        exit(1);
    if (ctx2 != NULL && ARG_PRECEDENCE("x-before-"))
        if (seccomp_load(ctx2) < 0)
            exit(1);

    seccomp_release(ctx);
    seccomp_release(ctx2);

    /* execute the syscall */
    errno = 0;
    if (sinfo.exec(&sinfo) < 0)
        exit(1);

    /* if we're still alive, print the results and return 0 */
    printf("%d %d %d %hd\n", sinfo.ret_val, sinfo.ret_errno,
                             sinfo.success(&sinfo), TESTING_SCERRNO);

    /* call syscall cleanup, if it exists */
    if (sinfo.cleanup)
        if (sinfo.cleanup(&sinfo) < 0)
            exit(1);

    if (trapped)
        return 2;
    else
        return 0;
}
