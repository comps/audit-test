/*
 * this tool executes a command within a cgroup, given a path to the "tasks"
 * file within a specific cgroup on a 'cgroup' filesystem type
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main(int argc, char **argv)
{
    FILE *f;

    if (argc < 3) {
        printf("usage: %s <path_to_tasks> <cmd> [args]\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    /* enter the requested cgroup */
    f = fopen(argv[1], "w");
    if (f == NULL) {
        perror("fopen");
        exit(EXIT_FAILURE);
    }
    fprintf(f, "%d\n", getpid());
    fclose(f);

    if (execvp(argv[2], argv+2) == -1) {
        perror("execvp");
        exit(EXIT_FAILURE);
    }

    /* execve shouldn't return */
    return EXIT_FAILURE;
}

/* vim: set sts=4 sw=4 et : */
