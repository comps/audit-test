/*
 *  test program to check whether spares files rare realy spares
 *
 * algorithm:
 * Create a file with defined data at beginning
 * and after a long lseek with defined data at
 * the end
 * Test: after write and reopen, check wether
 * contents of the file is without additional data
 *
 * looping over various paths that may provide access to
 * different file systems
 * /tmp -> ext3
 * /dev/shm -> tmpfs
 * /boot -> vfat (on Itanium)
 *
 */

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#define DATA_BEGIN "Blafasel"
#define DATA_END "Faselbla"
#define SEEK 10000000

#define RESULT '\0'

int test(char *);

int main(int argc, char **argv) {
    const char *filename = "/sparse_file";
    char *path;
	int ret=0;

    if(argc != 2) {
        fprintf(stderr, "No test path given as argument!\n");
        exit(1);
    }

    path = malloc(sizeof(char)*(strlen(argv[1])+strlen(filename))+1);
    strcpy(path, argv[1]);
    strcat(path, filename);
    ret = test(path);
    free(path);

	if(ret) {
        return ret;
    }

    return 0;
}

int test(char *file) {
	int fd;
	long ret, i;
	char buf1[sizeof(DATA_BEGIN)];
	char buf2[sizeof(DATA_END)];
	char buf3[1];
    extern int errno;

	snprintf(buf1, sizeof(DATA_BEGIN), DATA_BEGIN);
	snprintf(buf2, sizeof(DATA_END), DATA_END);

	printf("Testing file %s\n", file);

	unlink(file);

	fd = open(file, O_WRONLY|O_CREAT|O_EXCL, S_IRWXU);
	if(fd == -1) {
		printf("open of file failed: %s\n", strerror(errno));
		return 1;
	}
	ret = write(fd, buf1, sizeof(buf1));
	if(ret == -1) {
		printf("write begin failed: %s\n", strerror(errno));
		return 3;
	}
	ret = lseek(fd, SEEK, SEEK_SET);
	if(ret == -1) {
		printf("lseek failed: %s\n", strerror(errno));
		return 2;
	}
	ret = write(fd, buf2, sizeof(buf2));
	if(ret == -1) {
		printf("write end failed\n");
		return 4;
	}
	close(fd);
	sync();
	sync();
	sync();

	/******************/

	fd = open(file, O_RDONLY);
	ret = read(fd, buf1, sizeof(buf1));
	if(ret == -1) {
		printf("read begin failed\n");
		return 5;
	}

	for (i=sizeof(DATA_BEGIN); i<SEEK; i++) {
		ret = lseek(fd, i, SEEK_SET);
		if(ret == -1) {
			printf("lseek failed\n");
			return 6;
		}
		ret = read(fd, buf3, 1);
		if(ret != 1) {
			printf("read in lseek failed: %ld\n", ret);
			return 7;
		}
		if(buf3[0] != RESULT) {
			printf("data %i does not mach expected result\n", buf3[0]);
			return -1;
		}
	}
	ret = read(fd, buf2, sizeof(buf2));
	if(ret != sizeof(buf2)) {
		printf("read begin failed\n");
		return 8;
	}
	if(!(strncmp(buf2, DATA_BEGIN, sizeof(buf2)) && strncmp(buf2, DATA_END, sizeof(buf2)))) {
		printf ("Path %s: Test succedeed\n", file);
		close(fd);
		unlink(file);
		return 0;
	} else {
		printf("Path %s: Test failed\n", file);
		close(fd);
		unlink(file);
		return -1;
	}
}
