/*
 * this tool is used to stress the system as much as possible in certain areas
 * such as CPU usage, memory usage, etc., in order to test limits imposed by
 * cgroups
 * if needed, this binary should be generic enough for usage outside cgroup
 * testing
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <limits.h>
#include <errno.h>
#include <signal.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/time.h>
#include <sys/ioctl.h>
#include <linux/fs.h>

/*
 * helpers
 */

/* see ISO/IEC 9899, 7.14.1.1, p5 */
volatile sig_atomic_t run = 1;
void stop_loop(int signum) { run = 0; }
void do_nothing(int signum) { }
int int_action(void (*handler)(int))
{
    return sigaction(SIGINT, &(struct sigaction){.sa_handler=handler}, NULL);
}

/*
 * testing functions
 */

typedef unsigned long long ull;

/* eat cpu cycles */
ull test_cpu(void)
{
    volatile ull i;

    int_action(stop_loop);

    /* may stress RAM as well, but as far as OS resources go, only CPU */
    for (i = 0; run && i < ULLONG_MAX; i++);

    return i;
}

/* eat memory (heap) */
ull test_memory(ull size)
{
    char *ptr;
    ull i;

    int_action(stop_loop);

    ptr = mmap(NULL, size, PROT_WRITE, MAP_PRIVATE|MAP_ANONYMOUS|MAP_NORESERVE,
               -1, 0);
    if (ptr == MAP_FAILED)
        return 0;

    /* actually "use" the memory */
    for (i = 0; run && i < size; i++)
        ptr[i] = 0x01;

    return i;
}

/* read blocks from opened block device, at_once blocks at a time */
ull test_blkio(int fd, ull at_once)
{
    char *buff;
    int blksz;
    ssize_t len;
    ull total_len;

    if (!at_once)
        return 0;

    if (ioctl(fd, BLKBSZGET, &blksz) == -1)
        return 0;

    at_once = at_once * blksz;  /* sectors -> bytes */
    buff = malloc(at_once);
    if (!buff)
        return 0;

    int_action(stop_loop);

    total_len = 0;
    while (run && (len = read(fd, buff, at_once)) > 0)
        total_len += len;

    return total_len;
}

/* test the devices cgroups controller */
int test_devices(char *path)
{
    int fd;

    errno = 0;
    /* we do open() here because it is part of the test, not "setup" */
    fd = open(path, O_RDWR);
    if (fd == -1)
        return errno;

    close(fd);

    return 0;
}

/* test the freezer cgroups controller */
void test_freezer(int fd)
{
    int_action(do_nothing);

    /* slight race condition possible (freezer1 written, not yet in pause()) */
    write(fd, "freezer1\n", 9);
    pause();
    write(fd, "freezer2\n", 9);
}

int main(int argc, char **argv)
{
    if (argc < 2) {
        fprintf(stderr,
                "usage: %s <controller> [arguments]\n"
                "\n"
                "Arguments:\n"
                "  controller: cgroups controller to test\n"
                "  arguments:  zero or more controller-specific arguments\n"
                "\n"
                "Controllers (with arguments):\n"
                "  cpu\n"
                "  memory size_in_bytes\n"
                "  blkio path_to_device [blocks_at_a_time]\n"
                "  devices path_to_device\n"
                "  freezer\n"
                , argv[0]);
        exit(EXIT_FAILURE);
    }

    if (!strcmp(argv[1], "cpu")) {
        printf("%llu\n", test_cpu());

    } else if (!strcmp(argv[1], "memory")) {
        if (argc < 3)
            exit(EXIT_FAILURE);
        ull size = strtoull(argv[2], NULL, 10);
        printf("%llu\n", test_memory(size));

    } else if (!strcmp(argv[1], "blkio")) {
        if (argc < 3)
            exit(EXIT_FAILURE);
        int fd = open(argv[2], O_RDONLY);
        if (fd == -1) {
            perror("open");
            exit(EXIT_FAILURE);
        }
        ull blocks = 1;
        if (argc >= 4)
            blocks = strtoull(argv[3], NULL, 10);
        printf("%llu\n", test_blkio(fd, blocks));
        close(fd);

    } else if (!strcmp(argv[1], "devices")) {
        if (argc < 3)
            exit(EXIT_FAILURE);
        printf("%d\n", test_devices(argv[2]));

    } else if (!strcmp(argv[1], "freezer")) {
        test_freezer(STDOUT_FILENO);

    } else {
        fprintf(stderr, "unsupported controller\n");
        exit(EXIT_FAILURE);
    }

    return EXIT_SUCCESS;
}

/* vim: set sts=4 sw=4 et : */
