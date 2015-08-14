#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <selinux/selinux.h>
#include <selinux/context.h>
int main(int argc, char **argv)
{
    security_context_t ctxs = NULL;
    context_t ctx = NULL;

    /* eat up stdin */
    if (getenv("UNDER_INETD"))
        while ((getc(stdin)) != EOF);

    if (getcon_raw(&ctxs) == -1) {
        perror("getcon");
        return 1;
    }

    if (argc >= 2) {
        ctx = context_new(ctxs);
        if (!strcmp(argv[1], "user")) {
            printf("%s\n", context_user_get(ctx));
        } else if (!strcmp(argv[1], "role")) {
	    printf("%s\n", context_role_get(ctx));
        } else if (!strcmp(argv[1], "type")) {
	    printf("%s\n", context_type_get(ctx));
        } else if (!strcmp(argv[1], "range")) {
	    printf("%s\n", context_range_get(ctx));
        }
	context_free(ctx);
    } else {
        printf("%s\n", ctxs);
    }

    freecon(ctxs);
    return 0;
}
