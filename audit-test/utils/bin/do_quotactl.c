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

#include "includes.h"
#include <sys/quota.h>
#include <xfs/xqm.h>

int main(int argc, char **argv)
{
    int exitval, result;
    int cmd, id;

    if (argc != 4) {
        fprintf(stderr, "Usage:\n%s <normal|xfs> <special> <user_id>\n"
            "Gets disk quota limits for user id\n", argv[0]);
        return TEST_ERROR;
    }

    id = strtol(argv[3], NULL, 10);

    errno = 0;
    if (!strcmp(argv[1], "xfs")) {
        struct fs_disk_quota quota;
        cmd = QCMD(Q_XGETQUOTA, USRQUOTA);
        exitval = quotactl(cmd, argv[2], id, (void *) &quota);
    } else if (!strcmp(argv[1], "normal")) {
        struct dqblk quota;
        cmd = QCMD(Q_GETQUOTA, USRQUOTA);
        exitval = quotactl(cmd, argv[2], id, (void *) &quota);
    } else {
        fprintf(stderr, "Invalid argument: only normal or xfs\n");
        return TEST_ERROR;
    }
    result = exitval < 0;

    fprintf(stderr, "%d %d %d\n", result, result ? errno : exitval, getpid());
    return result;
}
