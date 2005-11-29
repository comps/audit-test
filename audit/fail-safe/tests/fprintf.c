#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

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
