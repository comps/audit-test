/*  (c) Copyright Hewlett-Packard Development Company, L.P., 2007
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of version 2 the GNU General Public License as
 *  published by the Free Software Foundation.
 *  
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *  
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#define IPC_MODULE
#include "do_msgctl.c"
#include "do_msgget.c"
#include "do_msgrcv.c"
#include "do_msgsnd.c"
#include "do_semctl.c"
#include "do_semget.c"
#include "do_semop.c"
#include "do_semtimedop.c"
#include "do_shmat.c"
#include "do_shmctl.c"
#include "do_shmget.c"

int main(int argc, char **argv)
{
    char *op;

    if (argc < 2) {
        fprintf(stderr, "%s: you must specify an ipc operation\n", argv[0]);
        return 1;
    }

    op = argv[1];
    argv++;
    argc--;

    if (!strcmp(op, "msgctl")) {
        return do_msgctl(argc, argv);
    } else if (!strcmp(op, "msgget")) {
        return do_msgget(argc, argv);
    } else if (!strcmp(op, "msgrcv")) {
        return do_msgrcv(argc, argv);
    } else if (!strcmp(op, "msgsnd")) {
        return do_msgsnd(argc, argv);
    } else if (!strcmp(op, "semctl")) {
        return do_semctl(argc, argv);
    } else if (!strcmp(op, "semget")) {
        return do_semget(argc, argv);
    } else if (!strcmp(op, "semop")) {
        return do_semop(argc, argv);
    } else if (!strcmp(op, "semtimedop")) {
        return do_semtimedop(argc, argv);
    } else if (!strcmp(op, "shmat")) {
        return do_shmat(argc, argv);
    } else if (!strcmp(op, "shmctl")) {
        return do_shmctl(argc, argv);
    } else if (!strcmp(op, "shmget")) {
        return do_shmget(argc, argv);
    } else {
        fprintf(stderr, "%s: unsupported ipc operation: %s\n", argv[0], op);
        return 1;
    }
}
