#include "includes.h"
#include <sys/ipc.h>
#include <sys/sem.h>

typedef struct sembuf_s {
    short sem_num;
    short sem_op;
    short sem_flg;
} sembuf_t;

int main(int argc, char **argv)
{
    // I'm alive!!

    int semid;
    struct sembuf sembuf;

    if (argc != 2) {
	fprintf(stderr, "Wrong number of arguments [%d].  Must pass in the semaphore identifier.\n", argc);
	printf("argv[1] = [%s]\n", argv[1]);
	exit(-1);
    }

    semid = atoi(argv[1]);

    sembuf.sem_num = 0;
    sembuf.sem_op = 1;		// Post to the semaphore
    sembuf.sem_flg = 0;

    semop(semid, &sembuf, 1);
    return 0;

}
