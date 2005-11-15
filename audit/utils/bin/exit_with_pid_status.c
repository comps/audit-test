#include "includes.h"

int main(int argc, char **argv)
{

    FILE *fd = NULL;

    if (argc < 2)
	return -1;
    fd = fopen(argv[1], "w+");

    if (fd == NULL)
	return -errno;

    fprintf(fd, "%d", getpid());

    fclose(fd);

    return 0;

}
