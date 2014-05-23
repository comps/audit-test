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
#include <pwd.h>
#include <grp.h>

int main(int argc, char **argv)
{
    int exitval, result;
    long id_read;
    char *endptr;
    uid_t uid = -1;
    gid_t gid = -1;
    struct passwd *pw;
    struct group *gr;

    if (argc < 3) {
        fprintf(stderr, "Usage:\n%s <path> <owner> [group]\n", argv[0]);
        return TEST_ERROR;
    }

    /* try to convert owner/group into numeric values,
     * if it fails with EINVAL, the owner/group is probably given
     * as name (and to be resolved via get*nam) */

    /* uid */
    errno = 0;
    id_read = strtol(argv[2], &endptr, 10);
    if (errno || endptr == argv[2] || id_read > USHRT_MAX) {
        errno = 0;
        pw = getpwnam(argv[2]);
        if (!pw) {
            fprintf(stderr, "do_lchown: getpwnam: %s\n",
                    errno ? strerror(errno) : "no entry found");
            return TEST_ERROR;
        }
        uid = pw->pw_uid;
    } else {
        uid = id_read;
    }

    /* gid */
    if (argc > 3) {
        errno = 0;
        id_read = strtol(argv[3], &endptr, 10);
        if (errno || endptr == argv[3] || id_read > USHRT_MAX) {
            gr = getgrnam(argv[3]);
            if (!gr) {
                fprintf(stderr, "do_lchown: getgrnam: %s\n",
                        errno ? strerror(errno) : "no entry found");
                return TEST_ERROR;
            }
            gid = gr->gr_gid;
        } else {
            gid = id_read;
        }
    }

    errno = 0;
    exitval = syscall(__NR_lchown, argv[1], uid, gid);
    result = exitval < 0;

    fprintf(stderr, "%d %d %d\n", result, result ? errno : exitval, getpid());
    return result;
}
