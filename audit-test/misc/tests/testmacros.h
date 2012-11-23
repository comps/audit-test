#ifndef _TESTMACROS_H
#define _TESTMACROS_H

#include <stdio.h>
#include <stdlib.h>

#define _MSGN fprintf(stderr, "FAIL:%s:%d: ", __FILE__, __LINE__)
#define _MSG(s) fprintf(stderr, "FAIL:%s:%d: %s\n", __FILE__, __LINE__, s)

#define SYSCALL(sys) do { if ((sys) < 0) { _MSGN; perror(#sys); exit(1); } } while(0)

#define DIE_IF(cond) do { if ((cond)) { _MSG(#cond); exit(1); } } while(0)

#define DIE_UNLESS(cond) do { if (!(cond)) { _MSG(#cond); exit(1); } } while(0)

#endif /* TESTMACROS_H */
