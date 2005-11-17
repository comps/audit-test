/* =======================================================================
 * (c) Copyright Hewlett-Packard Development Company, L.P., 2005
 * 
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License.
 * 
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 * 
 *   You should have received a copy of the GNU General Public License
 *   with this package; if not, write to the Free Software Foundation,
 *   Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 * 
 * ======================================================================= 
 */
#include "includes.h"
#include <pwd.h>

uid_t gettestuid(void)
{
    struct passwd *pw;

    pw = getpwnam(TEST_USER);
    if (!pw) {
        fprintf(stderr, "Error: unable to get passwd info for test user (%s)\n",
                TEST_USER);
        return -1;
    }
    return pw->pw_uid;
}

gid_t gettestgid(void)
{
    struct passwd *pw;

    pw = getpwnam(TEST_USER);
    if (!pw) {
        fprintf(stderr, "Error: unable to get passwd info for test user (%s)\n",
                TEST_USER);
        return -1;
    }
    return pw->pw_gid;
}

int setuidresgid_root()
{
    int rc = 0;

    errno = 0;
    rc = seteuid(0);
    if (rc < 0) {
        fprintf(stderr, "Error: seteuid() to root: %s\n", strerror(errno));
        goto exit;
    }

    errno = 0;
    rc = setresgid(0, 0, 0);
    if (rc < 0)
        fprintf(stderr, "Error: setresgid() to root: %s\n", strerror(errno));


exit:
    return rc;
}

int setuidresgid_test()
{
    int rc = 0;
    int uid = gettestuid();
    int gid = gettestgid();

    if ((uid < 0) || (gid < 0)) {
        rc = -1;
        goto exit;
    }

    errno = 0;
    rc = setresgid(gid, gid, gid);
    if (rc < 0) {
        fprintf(stderr, "Error: setresgid() to %s: %s\n", TEST_USER, 
                strerror(errno));
        goto exit;
    }

    errno = 0;
    rc = seteuid(uid);
    if (rc < 0)
        fprintf(stderr, "Error: seteuid() to %s: %s\n", TEST_USER, 
                strerror(errno));

exit:
    return rc;
}
