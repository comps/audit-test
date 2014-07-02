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

#include "includes.h"

#include "ipc_common.c"

int main(int argc, char **argv)
{
    int exitval, result;
    struct msqid_ds buf;
    int msqid, cmd;

    if (argc != 3) {
        fprintf(stderr, "Usage:\n%s <msqid> <cmd>\n", argv[0]);
        return 1;
    }

    msqid = atoi(argv[1]);
    cmd = atoi(argv[2]);

    errno = 0;

    switch (cmd) {
    case IPC_RMID:
        exitval = msgctl(msqid, cmd, NULL);
        break;
    case IPC_SET:
        memset(&buf, 0, sizeof(buf));
        buf.msg_perm.uid = 0; /* use root's uid */
        exitval = msgctl(msqid, cmd, &buf);
        break;
    case IPC_STAT:
        exitval = msgctl(msqid, cmd, &buf);
        break;
    default:
        exitval = -1;
        break;
    }

    result = exitval < 0;

    fprintf(stderr, "%d %d %d\n", result, result ? errno : exitval, getpid());
    return result;
}
