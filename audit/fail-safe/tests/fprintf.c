#include <dlfcn.h>
#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#if 0
static void *libc_handle = NULL;
int (*true_fprintf)(FILE *stream, const char *format, ...);

void __attribute__ ((constructor)) libfprintf_init(void)
{
    printf("Initializing libfprintf\n");

    libc_handle = dlopen("libc.so.6.1", RTLD_LAZY);
    if (!libc_handle) {
        printf("libfprintf: can't dlopen libc: %s\n", dlerror());
        exit(9);
    }

    true_fprintf = dlsym(libc_handle, "fprintf");
    if (!true_fprintf) {
        printf("libfprintf: can't resolve fprintf: %s\n", dlerror());
        exit(9);
    }
}
#endif

int fprintf(FILE *stream, const char *format, ...)
{
    va_list ap;
    int status;
    char *s;
    static int fail_after = 0;

    if (strcmp(format, "%s\n") == 0) {
        va_start(ap, format);
        s = va_arg(ap, char*);
        va_end(ap);

        if (strncmp(s, "type=USER", 9) == 0 &&
            fail_after++ == 20)
        {
            errno = EIO;
            return -1;
        }
    }

    va_start(ap, format);
    status = vfprintf(stream, format, ap);
    va_end(ap);
    return status;
}
