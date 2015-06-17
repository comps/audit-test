/* Copyright (c) 2014 Red Hat, Inc. All rights reserved.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of version 2 the GNU General Public License as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sched.h>

/* this could be reduced to less than half the code if we did setns() right
 * after open() and didn't keep any metadata - however that has a disadvantage
 * - once you setns(), you can't reach handles that are now hidden from you
 * (like mount --bind on-disk handles after setns to a new private mount ns,
 *  like trying to reach /proc/<pid>/ns after changing pid ns, ...)
 * - the solution is to open() everything first and then do setns()
 */
struct nsholder {
    int fd;
    int nstype;
    struct nsholder *next;
};

void print_help(void)
{
    printf(
        "setns - a simple setns(2) wrapper\n"
        "\n"
        "Usage: setns [options] <cmd> [args]\n"
        "\n"
        "Options:\n"
        "    -a handle            switch to any namespace handle\n"
        "    -m|u|i|n|p|U handle  check ns type before switching\n"
        "                         (mnt, uts, ipc, net, pid, user)\n"
        "\n"
        "Multiple options can be specified - all will be processed\n"
        "(open() + setns() + close()) in a specified order.\n"
        );
}

struct nsholder *advance_and_free(struct nsholder *nsh)
{
    struct nsholder *oldnsh;
    oldnsh = nsh;
    nsh = nsh->next;
    free(oldnsh);
    return nsh;
}

void add_ns(struct nsholder *nsh, char *handle, int nstype)
{
    struct nsholder *newh;
    int fd;

    fd = open(handle, O_RDONLY);
    if (fd == -1) {
        perror("open");
        exit(EXIT_FAILURE);
    }

    while (nsh->next != NULL)
        nsh = nsh->next;

    newh = malloc(sizeof(struct nsholder));
    if (newh == NULL) {
        perror("malloc");
        exit(EXIT_FAILURE);
    }

    newh->fd = fd;
    newh->nstype = nstype;
    newh->next = NULL;

    nsh->next = newh;
}

void call_setns(struct nsholder *nsh)
{
    while (nsh != NULL) {
        if (setns(nsh->fd, nsh->nstype) == -1) {
            perror("setns");
            exit(EXIT_FAILURE);
        }
        close(nsh->fd);
        nsh = advance_and_free(nsh);
    }
}

int main(int argc, char **argv)
{
    int c;
    struct nsholder *ns_set;

    /* alloc list head */
    ns_set = calloc(1, sizeof(struct nsholder));
    if (ns_set == NULL) {
        perror("malloc");
        exit(EXIT_FAILURE);
    }

    while ((c = getopt(argc, argv, "+a:m:u:i:n:p:U:")) != -1) {
        switch (c) {
            case 'a':
                add_ns(ns_set, optarg, 0);
                break;
            case 'm':
                add_ns(ns_set, optarg, CLONE_NEWNS);
                break;
            case 'u':
                add_ns(ns_set, optarg, CLONE_NEWUTS);
                break;
            case 'i':
                add_ns(ns_set, optarg, CLONE_NEWIPC);
                break;
            case 'n':
                add_ns(ns_set, optarg, CLONE_NEWNET);
                break;
            case 'p':
                add_ns(ns_set, optarg, CLONE_NEWPID);
                break;
            case 'U':
                add_ns(ns_set, optarg, CLONE_NEWUSER);
                break;
            case ':':
            case '?':
                print_help();
                exit(EXIT_FAILURE);
        }
    }

    argc -= optind-1;
    argv += optind-1;

    if (argc < 2) {
        print_help();
        exit(EXIT_FAILURE);
    }

    /* move ns_set to the first valid entry (instead of zero'd list head)
     * and call setns() on every list item, freeing it after use */
    ns_set = advance_and_free(ns_set);
    if (ns_set != NULL)
        call_setns(ns_set);

    if (execvp(argv[1], argv+1) == -1) {
        perror("execvp");
        exit(EXIT_FAILURE);
    }

    /* execve shouldn't return */
    return EXIT_FAILURE;
}

/* vim: set sts=4 sw=4 et : */
