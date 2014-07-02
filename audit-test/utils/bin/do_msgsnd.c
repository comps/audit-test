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
#include <sys/msg.h>

int main(int argc, char **argv)
{
    int exitval, result;
    struct msgbuf *buf;
    int buflen;

    int msqid;
    long msgtyp;
    char *msg;

    if (argc != 4) {
        fprintf(stderr, "Usage:\n%s <msqid> <msgtyp> <msg_data>\n", argv[0]);
        return 1;
    }

    msqid = atoi(argv[1]);
    msgtyp = atoi(argv[2]);
    msg = argv[3];

    buflen = sizeof(struct msgbuf) + strlen(msg) + 1;
    buf = (struct msgbuf *)malloc(buflen);
    if (!buf) {
        perror("malloc");
        return 1;
    }
    buf->mtype = msgtyp;
    strcpy(buf->mtext, msg);

    errno = 0;
    exitval = msgsnd(msqid, buf, buflen, IPC_NOWAIT);
    result = exitval < 0;

    free(buf);

    fprintf(stderr, "%d %d %d\n", result, result ? errno : exitval, getpid());
    return result;
}
